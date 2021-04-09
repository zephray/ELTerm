//
// Copyright 2021 Wenting Zhang <zephray@outlook.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/pll.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "eldata.pio.h"
#include "graphics.h"
#include "el.h"

PIO el_pio = pio0;
// Uses 3 DMA channels: The RP2040 DMA doesn't support list, so using chainning
// to implement address wrapping around. 2 DMA channels are for 2 half-screens,
// the last is for the half screen that crosses the boundary.
int el_udma_chan, el_ldma_chan, el_wrap_chan;

unsigned char framebuf_bp0[SCR_STRIDE * SCR_BUF_HEIGHT];
unsigned char framebuf_bp1[SCR_STRIDE * SCR_BUF_HEIGHT];

static int frame_state = 0;
volatile int frame_scroll_lines = 0;
volatile bool frame_sync = false;

static void el_sm_load_reg(uint sm, enum pio_src_dest dst, uint32_t val) {
    pio_sm_put_blocking(el_pio, sm, val);
    pio_sm_exec(el_pio, sm, pio_encode_pull(false, false));
    pio_sm_exec(el_pio, sm, pio_encode_out(dst, 32));
}

static void el_sm_load_isr(uint sm, uint32_t val) {
    el_sm_load_reg(sm, pio_isr, val);
}

static void el_dma_init_channel(uint chan, uint dreq, volatile uint32_t *dst) {
    dma_channel_config c = dma_channel_get_default_config(chan);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, false);
    channel_config_set_dreq(&c, dreq);

    dma_channel_configure(chan, &c, dst, NULL, 0, false);
}

static void el_dma_config_for_udata(uint chan) {
    el_dma_init_channel(chan, DREQ_PIO0_TX0 + EL_UDATA_SM, &el_pio->txf[EL_UDATA_SM]);
}

static void el_dma_config_for_ldata(uint chan) {
    el_dma_init_channel(chan, DREQ_PIO0_TX0 + EL_LDATA_SM, &el_pio->txf[EL_LDATA_SM]);
}

static void el_dma_config_chainning(uint chan, uint chain_to) {
    dma_channel_config c = dma_get_channel_config(chan);
    channel_config_set_chain_to(&c, chain_to);
    dma_channel_set_config(chan, &c, false);
}

static void el_pio_irq_handler() {
    uint8_t *framebuf = frame_state ? framebuf_bp0 : framebuf_bp1;
    frame_state = !frame_state;

    if (frame_scroll_lines >= SCR_BUF_HEIGHT) {
        frame_scroll_lines = frame_scroll_lines % SCR_BUF_HEIGHT;
    }

    if (frame_scroll_lines <= SCR_ADD_HEIGHT) {
        // Upper screen: 0-239 to 16-255, Lower screen: 240-479 to 256-495, 2 DMA used
        uint32_t *rdptr_ud = (uint32_t *)(framebuf + SCR_STRIDE * frame_scroll_lines);
        uint32_t *rdptr_ld = (uint32_t *)(framebuf + SCR_STRIDE * frame_scroll_lines + SCR_STRIDE * SCR_HEIGHT / 2);
        dma_channel_set_read_addr(el_udma_chan, rdptr_ud, false);
        dma_channel_set_read_addr(el_ldma_chan, rdptr_ld, false);
        dma_channel_set_trans_count(el_udma_chan, SCR_STRIDE_WORDS * SCR_HEIGHT / 2, false);
        dma_channel_set_trans_count(el_ldma_chan, SCR_STRIDE_WORDS * SCR_HEIGHT / 2, false);
        el_dma_config_chainning(el_udma_chan, el_udma_chan);
        el_dma_config_chainning(el_ldma_chan, el_ldma_chan);
    }
    else if (frame_scroll_lines < (240 + SCR_ADD_HEIGHT)) {
        // 3 DMA used, upper screen uses 1 DMA, lower screen uses 2 DMA
        uint32_t *rdptr_ud = (uint32_t *)(framebuf + SCR_STRIDE * frame_scroll_lines);
        uint32_t *rdptr_ld = (uint32_t *)(framebuf + SCR_STRIDE * frame_scroll_lines + SCR_STRIDE * SCR_HEIGHT / 2);
        el_dma_config_for_ldata(el_wrap_chan);
        dma_channel_set_read_addr(el_udma_chan, rdptr_ud, false);
        dma_channel_set_read_addr(el_ldma_chan, rdptr_ld, false);
        dma_channel_set_read_addr(el_wrap_chan, framebuf, false);
        dma_channel_set_trans_count(el_udma_chan, SCR_STRIDE_WORDS * SCR_HEIGHT / 2, false);
        dma_channel_set_trans_count(el_ldma_chan, SCR_STRIDE_WORDS * (SCR_HEIGHT / 2 + SCR_ADD_HEIGHT - frame_scroll_lines), false);
        dma_channel_set_trans_count(el_wrap_chan, SCR_STRIDE_WORDS * (frame_scroll_lines - SCR_ADD_HEIGHT), false);
        el_dma_config_chainning(el_udma_chan, el_udma_chan);
        el_dma_config_chainning(el_ldma_chan, el_wrap_chan);
    }
    else if (frame_scroll_lines == (240 + SCR_ADD_HEIGHT)) {
        uint32_t *rdptr_ud = (uint32_t *)(framebuf + SCR_STRIDE * (SCR_HEIGHT / 2 + SCR_ADD_HEIGHT));
        uint32_t *rdptr_ld = (uint32_t *)framebuf;
        dma_channel_set_read_addr(el_udma_chan, rdptr_ud, false);
        dma_channel_set_read_addr(el_ldma_chan, rdptr_ld, false);
        dma_channel_set_trans_count(el_udma_chan, SCR_STRIDE_WORDS * SCR_HEIGHT / 2, false);
        dma_channel_set_trans_count(el_ldma_chan, SCR_STRIDE_WORDS * SCR_HEIGHT / 2, false);
        el_dma_config_chainning(el_udma_chan, el_udma_chan);
        el_dma_config_chainning(el_ldma_chan, el_ldma_chan);
    }
    else {
        // Upper screen: from 241-0 to 479-238, lower screen: from 1-240 to 239-478
        // 3 DMA used, upper screen uses 2 DMA, lower screen uses 1 DMA
        uint32_t *rdptr_ud = (uint32_t *)(framebuf + SCR_STRIDE * frame_scroll_lines);
        uint32_t *rdptr_ld = (uint32_t *)(framebuf + SCR_STRIDE * (frame_scroll_lines - SCR_HEIGHT / 2 - SCR_ADD_HEIGHT));
        el_dma_config_for_udata(el_wrap_chan);
        dma_channel_set_read_addr(el_udma_chan, rdptr_ud, false);
        dma_channel_set_read_addr(el_ldma_chan, rdptr_ld, false);
        dma_channel_set_read_addr(el_wrap_chan, framebuf, false);
        dma_channel_set_trans_count(el_udma_chan, SCR_STRIDE_WORDS * (SCR_BUF_HEIGHT - frame_scroll_lines), false);
        dma_channel_set_trans_count(el_ldma_chan, SCR_STRIDE_WORDS * SCR_HEIGHT / 2, false);
        dma_channel_set_trans_count(el_wrap_chan, SCR_STRIDE_WORDS * (frame_scroll_lines - SCR_HEIGHT / 2 - SCR_ADD_HEIGHT), false);
        el_dma_config_chainning(el_udma_chan, el_wrap_chan);
        el_dma_config_chainning(el_ldma_chan, el_ldma_chan);
    }

    pio_sm_set_enabled(el_pio, EL_UDATA_SM, false);
    pio_sm_set_enabled(el_pio, EL_LDATA_SM, false);

    pio_sm_clear_fifos(el_pio, EL_UDATA_SM);
    pio_sm_clear_fifos(el_pio, EL_LDATA_SM);

    pio_sm_restart(el_pio, EL_UDATA_SM);
    pio_sm_restart(el_pio, EL_LDATA_SM);

    // Load configuration values
    el_sm_load_reg(EL_UDATA_SM, pio_y, SCR_REFRESH_LINES - 2);
    el_sm_load_reg(EL_UDATA_SM, pio_isr, SCR_LINE_TRANSFERS - 1);
    el_sm_load_reg(EL_LDATA_SM, pio_isr, SCR_LINE_TRANSFERS - 1);

    // Setup DMA
    dma_channel_start(el_udma_chan);
    dma_channel_start(el_ldma_chan);
    // Clear IRQ flag
    el_pio->irq = 0x02;
    // start SM
    pio_enable_sm_mask_in_sync(el_pio,
            (1u << EL_UDATA_SM) | (1u << EL_LDATA_SM));

    frame_sync = 1;
}

static void el_sm_init() {
    static uint udata_offset, ldata_offset;

    for (int i = 0; i < 4; i++) {
        pio_gpio_init(el_pio, UD0_PIN + i);
        pio_gpio_init(el_pio, LD0_PIN + i);
    }
    pio_gpio_init(el_pio, PIXCLK_PIN);
    pio_gpio_init(el_pio, HSYNC_PIN);
    pio_gpio_init(el_pio, VSYNC_PIN);
    pio_sm_set_consecutive_pindirs(el_pio, EL_UDATA_SM, UD0_PIN, 4, true);
    pio_sm_set_consecutive_pindirs(el_pio, EL_UDATA_SM, PIXCLK_PIN, 1, true);
    pio_sm_set_consecutive_pindirs(el_pio, EL_UDATA_SM, VSYNC_PIN, 1, true);
    pio_sm_set_consecutive_pindirs(el_pio, EL_LDATA_SM, LD0_PIN, 4, true);
    pio_sm_set_consecutive_pindirs(el_pio, EL_LDATA_SM, HSYNC_PIN, 1, true);

    udata_offset = pio_add_program(el_pio, &el_udata_program);
    ldata_offset = pio_add_program(el_pio, &el_ldata_program);

    //printf("EL USM offset: %d, EL LSM offset: %d\n", udata_offset, ldata_offset);

    int cycles_per_pclk = 2;
    float div = clock_get_hz(clk_sys) / (EL_TARGET_PIXCLK * cycles_per_pclk);

    pio_sm_config cu = el_udata_program_get_default_config(udata_offset);
    sm_config_set_sideset_pins(&cu, PIXCLK_PIN);
    sm_config_set_out_pins(&cu, UD0_PIN, 4);
    sm_config_set_set_pins(&cu, VSYNC_PIN, 1);
    sm_config_set_fifo_join(&cu, PIO_FIFO_JOIN_TX);
    sm_config_set_out_shift(&cu, true, true, 32);
    sm_config_set_clkdiv(&cu, div);
    pio_sm_init(el_pio, EL_UDATA_SM, udata_offset, &cu);

    pio_sm_config cl = el_ldata_program_get_default_config(ldata_offset);
    sm_config_set_set_pins(&cl, HSYNC_PIN, 1);
    sm_config_set_out_pins(&cl, LD0_PIN, 4);
    sm_config_set_fifo_join(&cl, PIO_FIFO_JOIN_TX);
    sm_config_set_out_shift(&cl, true, true, 32);
    sm_config_set_clkdiv(&cl, div);
    pio_sm_init(el_pio, EL_LDATA_SM, ldata_offset, &cl);

    el_pio->inte0 = PIO_IRQ0_INTE_SM1_BITS;
    irq_set_exclusive_handler(PIO0_IRQ_0, el_pio_irq_handler);
    irq_set_enabled(PIO0_IRQ_0, true);
}

static void el_dma_init() {
    el_udma_chan = dma_claim_unused_channel(true);
    el_dma_config_for_udata(el_udma_chan);

    el_ldma_chan = dma_claim_unused_channel(true);
    el_dma_config_for_ldata(el_ldma_chan);

    el_wrap_chan = dma_claim_unused_channel(true);
    // leave unused for now, will be configured once need to be used.
}

void el_start() {
    memset(framebuf_bp0, 0x00, SCR_STRIDE * SCR_HEIGHT);
    memset(framebuf_bp1, 0x00, SCR_STRIDE * SCR_HEIGHT);

    el_sm_init();
    el_dma_init();
    el_pio_irq_handler();
}

/*void el_debug() {
    printf("PIO USM PC: %d, LSM PC: %d, IRQ: %d\n", el_pio->sm[EL_UDATA_SM].addr, el_pio->sm[EL_LDATA_SM].addr, el_pio->irq);
}*/