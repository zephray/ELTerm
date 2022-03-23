#pragma once
// Dummy file for building on PC
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define GPIO_IN 0
#define GPIO_OUT 1

typedef struct repeating_timer repeating_timer_t;
typedef bool(* repeating_timer_callback_t) (repeating_timer_t *rt);

struct repeating_timer {
    int64_t delay_us;
    repeating_timer_callback_t callback;
    void *user_data;
};

bool add_repeating_timer_ms(int32_t delay_ms,
        repeating_timer_callback_t callback, void *user_data,
        repeating_timer_t *out);
uint32_t time_us_32();
void gpio_init(int pin);
void gpio_set_dir(int pin, int dir);
void gpio_put(int pin, int value);

void fake_picolib_tick(uint32_t tick);
