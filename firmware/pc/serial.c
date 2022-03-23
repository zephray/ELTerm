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
#include "../serial.h"

uint8_t serial_ringbuf[SERIAL_RIGNBUF_SIZE];
static volatile int rdptr = 0;
static volatile int wrptr = 0;

static int serial_get_free() {
    int diff = wrptr - rdptr;
    if (diff < 0)
        diff += SERIAL_RIGNBUF_SIZE;
    return diff;
}

void serial_rx_push(uint8_t ch) {
    if ((wrptr != (rdptr - 1)) && 
            (!((wrptr == SERIAL_RIGNBUF_SIZE - 1) && (rdptr == 0)))) {
        serial_ringbuf[wrptr] = ch;
        wrptr++;
        if (wrptr == SERIAL_RIGNBUF_SIZE) wrptr = 0;
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
    return false;
}

void serial_init() {
    return;
}
