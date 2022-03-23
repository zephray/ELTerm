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
// Emulated pico stdlib for ELterm
#include "pico/stdlib.h"

static bool timer_enabled = false;
static repeating_timer_t *timer;
static uint32_t last_fire = 0;
static uint32_t last_tick = 0;

bool add_repeating_timer_ms (int32_t delay_ms,
        repeating_timer_callback_t callback, void *user_data,
        repeating_timer_t *out) {
    timer = out;
    timer->delay_us = abs(delay_ms) * 1000;
    timer->callback = callback;
    timer->user_data = user_data;
    printf("Timer set to %d us\n", timer->delay_us);
    timer_enabled = true;
}

void fake_picolib_tick(uint32_t tick) {
    if (timer_enabled) {
        if ((tick - last_fire) > (timer->delay_us / 1000)) {
            timer->callback(timer);
            last_fire = tick;
        }
    }

    last_tick = tick;
}

uint32_t time_us_32() {
    return last_tick * 1000;
}

void gpio_init(int pin) {
    return;
}

void gpio_set_dir(int pin, int dir) {
    return;
}

void gpio_put(int pin, int value) {
    return;
}
