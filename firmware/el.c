#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/pll.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "eldata.pio.h"
#include "el.h"

PIO el_pio = pio0;
int el_udma_chan, el_ldma_chan;

unsigned char framebuf_bp0[SCR_STRIDE * SCR_HEIGHT];
unsigned char framebuf_bp1[SCR_STRIDE * SCR_HEIGHT];

static int frame_state = 0;

static void el_sm_load_reg(uint sm, enum pio_src_dest dst, uint32_t val) {
    pio_sm_put_blocking(el_pio, sm, val);
    pio_sm_exec(el_pio, sm, pio_encode_pull(false, false));
    pio_sm_exec(el_pio, sm, pio_encode_out(dst, 32));
}

static void el_sm_load_isr(uint sm, uint32_t val) {
    el_sm_load_reg(sm, pio_isr, val);
}

static void el_pio_irq_handler() {
    gpio_put(22, 1);

    uint8_t *framebuf = frame_state ? framebuf_bp0 : framebuf_bp1;
    frame_state = !frame_state;
    uint32_t *rdptr_ud = (uint32_t *)framebuf;
    uint32_t *rdptr_ld = (uint32_t *)(framebuf + SCR_STRIDE * SCR_HEIGHT / 2);
    dma_channel_set_read_addr(el_udma_chan, rdptr_ud, false);
    dma_channel_set_read_addr(el_ldma_chan, rdptr_ld, false);

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

    gpio_put(22, 0);
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
    dma_channel_config cu = dma_channel_get_default_config(el_udma_chan);
    channel_config_set_transfer_data_size(&cu, DMA_SIZE_32);
    channel_config_set_read_increment(&cu, true);
    channel_config_set_write_increment(&cu, false);
    channel_config_set_dreq(&cu, DREQ_PIO0_TX0 + EL_UDATA_SM);

    dma_channel_configure(el_udma_chan, &cu, &el_pio->txf[EL_UDATA_SM], NULL, SCR_STRIDE_WORDS * SCR_REFRESH_LINES, false);

    el_ldma_chan = dma_claim_unused_channel(true);
    dma_channel_config cl = dma_channel_get_default_config(el_ldma_chan);
    channel_config_set_transfer_data_size(&cl, DMA_SIZE_32);
    channel_config_set_read_increment(&cl, true);
    channel_config_set_write_increment(&cl, false);
    channel_config_set_dreq(&cl, DREQ_PIO0_TX0 + EL_LDATA_SM);

    dma_channel_configure(el_ldma_chan, &cl, &el_pio->txf[EL_LDATA_SM], NULL, SCR_STRIDE_WORDS * SCR_REFRESH_LINES, false);
}

void el_start() {
    memset(framebuf_bp0, 0x00, SCR_STRIDE * SCR_HEIGHT);
    memset(framebuf_bp1, 0x00, SCR_STRIDE * SCR_HEIGHT);

    gpio_init(22);
    gpio_set_dir(22, GPIO_OUT);

    el_sm_init();
    el_dma_init();
    el_pio_irq_handler();
}

/*void el_debug() {
    printf("PIO USM PC: %d, LSM PC: %d, IRQ: %d\n", el_pio->sm[EL_UDATA_SM].addr, el_pio->sm[EL_LDATA_SM].addr, el_pio->irq);
}*/