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
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "serial.h"

uint8_t serial_ringbuf[SERIAL_RIGNBUF_SIZE];
static volatile int rdptr = 0;
static volatile int wrptr = 0;

static int serial_get_free() {
    int diff = wrptr - rdptr;
    if (diff < 0)
        diff += SERIAL_RIGNBUF_SIZE;
    return diff;
}

void serial_rx_irq() {
    while (uart_is_readable(UART_ID)) {
        uint8_t ch = uart_getc(UART_ID);
        if ((wrptr != (rdptr - 1)) && 
                (!((wrptr == SERIAL_RIGNBUF_SIZE - 1) && (rdptr == 0)))) {
            serial_ringbuf[wrptr] = ch;
            wrptr++;
            if (wrptr == SERIAL_RIGNBUF_SIZE) wrptr = 0;
        }
    }
}

bool serial_getc(char *c) {
    if (serial_get_free() > 0) {
        *c = serial_ringbuf[rdptr];
        int new_rdptr = rdptr + 1;
        if (new_rdptr == SERIAL_RIGNBUF_SIZE)
            new_rdptr = 0;
        rdptr = new_rdptr;
        return true;
    }
    else {
        if (uart_is_readable(UART_ID)) {
            *c = uart_getc(UART_ID);
            return true;
        }
    }
    return false;
}

void serial_init() {
    uart_init(UART_ID, 2400);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    int actual = uart_set_baudrate(UART_ID, BAUD_RATE);

    uart_set_hw_flow(UART_ID, false, false);
    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);
    //uart_set_fifo_enabled(UART_ID, false);

    int UART_IRQ = UART_ID == uart0 ? UART0_IRQ : UART1_IRQ;

    irq_set_exclusive_handler(UART_IRQ, serial_rx_irq);
    irq_set_enabled(UART_IRQ, true);

    uart_set_irq_enables(UART_ID, true, false);
}

void serial_putc(char c) {
    uart_putc_raw(UART_ID, c);
}

void serial_puts(char *s) {
    // Unlike uart_puts, this does not do any CR/LF conversion
    while (*s) {
        uart_putc_raw(UART_ID, *s++);
    }
}