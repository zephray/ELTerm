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
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "el.h"
#include "graphics.h"
#include "terminal.h"
#include "serial.h"
#include "tusb.h"
#include "usbhid.h"
#include "termcore.h"

#define MAX_DEBUG_LEN 107
char debugmsg[MAX_DEBUG_LEN];

static TERM_STATE term_state_front_main;
static TERM_STATE *term_state_front = &term_state_front_main;

//Keyboard states
#define MAX_PRESSED_KEYS (6) // Limited by HID
uint8_t const keycode2ascii[128][2] = {HID_KEYCODE_TO_ASCII};
static uint8_t key_pressed_code[6] = {0};
static uint32_t key_pressed_since[6] = {0};
static bool key_is_shift = false;
static bool key_is_ctrl = false;

#define MAX_UPDATE (80 * 10)

static volatile bool timer_pending = false;

static bool cursor_state = false;

void term_clear_cursor() {
    int x = term_state_front->x;
    int y = term_state_front->y + term_state_front->y_offset;
    if (y >= TERM_BUF_HEIGHT) y -= TERM_BUF_HEIGHT;
    char text = term_state_front->textmap[y][x];
    char color = term_state_front->colormap[y][x];
    char fg = (uint8_t)color >> 4;
    char bg = color & 0xf;
    char flag = term_state_front->flagmap[y][x];
    graph_put_char(x * 8, y * 16, text, fg, bg, flag);
}

void term_disp_cursor() {
    int x = term_state_front->x;
    int y = term_state_front->y + term_state_front->y_offset;
    if (y >= TERM_BUF_HEIGHT) y -= TERM_BUF_HEIGHT;
    graph_fill_rect(x * 8, y * 16, (x + 1) * 8, (y + 1) * 16, COLOR_WHITE);
}

void term_update_cursor() {
    if (((cursor_state) || (mode_cursor_blinking == false)) && (mode_show_cursor == true)) {
        term_disp_cursor();
    }
    else {
        term_clear_cursor();
    }
}

bool term_timer_callback(struct repeating_timer *t) {
    timer_pending = true;
    return true;
}

void term_show_debug_message(char *str) {
    /*char c;
    int x = 1, y = 471;
    graph_fill_rect(0, 471, 640, 480, COLOR_WHITE);
    while (c = *str++) {
        graph_put_char_small(x, y, c, COLOR_BLACK, COLOR_WHITE);
        x += 6;
    }*/
    puts(str);
}

bool term_decode_special_keymode(uint8_t keycode, bool is_shift, bool is_ctrl) {
    if (mode_app_keypad) {
        /*if (keycode == HID_KEY_SPACE) {
            serial_puts("\eO ");
        }
        else if (keycode == HID_KEY_TAB) {
            serial_puts("\eOI");
        }
        else if (keycode == HID_KEY_RETURN) {
            serial_puts("\eOM");
        }*/
        if (keycode == HID_KEY_KEYPAD_MULTIPLY) {
            serial_puts("\eOj");
        }
        else if (keycode == HID_KEY_KEYPAD_ADD) {
            serial_puts("\eOk");
        }
        /*else if (keycode == HID_KEY_COMMA) {
            serial_puts("\eOl");
        }*/
        else if (keycode == HID_KEY_KEYPAD_SUBTRACT) {
            serial_puts("\eOm");
        }
        else if (keycode == HID_KEY_KEYPAD_DECIMAL) {
            serial_puts("\e[3~");
        }
        else if (keycode == HID_KEY_KEYPAD_DIVIDE) {
            serial_puts("\eOI");
        }
        else if (keycode == HID_KEY_KEYPAD_0) {
            serial_puts("\e[2~");
        }
        else if (keycode == HID_KEY_KEYPAD_1) {
            serial_puts("\eOF");
        }
        else if (keycode == HID_KEY_KEYPAD_2) {
            serial_puts("\e[B");
        }
        else if (keycode == HID_KEY_KEYPAD_3) {
            serial_puts("\e[6~");
        }
        else if (keycode == HID_KEY_KEYPAD_4) {
            serial_puts("\e[D");
        }
        else if (keycode == HID_KEY_KEYPAD_5) {
            serial_puts("\e[E");
        }
        else if (keycode == HID_KEY_KEYPAD_6) {
            serial_puts("\e[C");
        }
        else if (keycode == HID_KEY_KEYPAD_7) {
            serial_puts("\eOH");
        }
        else if (keycode == HID_KEY_KEYPAD_8) {
            serial_puts("\e[A");
        }
        else if (keycode == HID_KEY_KEYPAD_9) {
            serial_puts("\e[5~");
        }
        else if (keycode == HID_KEY_KEYPAD_EQUAL) {
            serial_puts("\eOX");
        }
        else
            return false;
    }
    else if (mode_app_cursor) {
        if (keycode == HID_KEY_ARROW_UP) {
            serial_puts("\eOA");
        }
        else if (keycode == HID_KEY_ARROW_DOWN) {
            serial_puts("\eOB");
        }
        else if (keycode == HID_KEY_ARROW_LEFT) {
            serial_puts("\eOC");
        }
        else if (keycode == HID_KEY_ARROW_RIGHT) {
            serial_puts("\eOD");
        }
        else if (keycode == HID_KEY_HOME) {
            serial_puts("\eOH");
        }
        else if (keycode == HID_KEY_END) {
            serial_puts("\eOF");
        }
        else
            return false;
    }
    return false;
}

void term_key_sendcode(uint8_t keycode, bool is_shift, bool is_ctrl) {
    uint8_t ch;
    if (term_decode_special_keymode(keycode, is_shift, is_ctrl))
        return;

    if (is_ctrl) {
        if (keycode == HID_KEY_SPACE) {
            ch = 0x00;
        }
        else {
            ch = keycode2ascii[keycode][1] & 0xbf;
        }
    }
    else {
        ch = keycode2ascii[keycode][is_shift ? 1 : 0];
    }

    if (keycode == HID_KEY_ARROW_UP) {
        serial_puts((is_ctrl) ? "\e[1;5A" : "\e[A");
    }
    else if (keycode == HID_KEY_ARROW_DOWN) {
        serial_puts((is_ctrl) ? "\e[1;5B" : "\e[B");
    }
    else if (keycode == HID_KEY_ARROW_RIGHT) {
        serial_puts((is_ctrl) ? "\e[1;5C" : "\e[C");
    }
    else if (keycode == HID_KEY_ARROW_LEFT) {
        serial_puts((is_ctrl) ? "\e[1;5D" : "\e[D");
    }
    else if (keycode == HID_KEY_F1) {
        serial_puts("\eOP");
    }
    else if (keycode == HID_KEY_F2) {
        serial_puts("\eOQ");
    }
    else if (keycode == HID_KEY_F3) {
        serial_puts("\eOR");
    }
    else if (keycode == HID_KEY_F4) {
        serial_puts("\eOS");
    }
    else if (keycode == HID_KEY_F5) {
        serial_puts("\e[15~");
    }
    else if (keycode == HID_KEY_F6) {
        serial_puts("\e[17~");
    }
    else if (keycode == HID_KEY_F7) {
        serial_puts("\e[18~");
    }
    else if (keycode == HID_KEY_F8) {
        serial_puts("\e[19~");
    }
    else if (keycode == HID_KEY_F9) {
        serial_puts("\e[20~");
    }
    else if (keycode == HID_KEY_F10) {
        serial_puts("\e[21~");
    }
    else if (keycode == HID_KEY_F11) {
        serial_puts("\e[23~");
    }
    else if (keycode == HID_KEY_F12) {
        serial_puts("\e[24~");
    }
    else if (keycode == HID_KEY_INSERT) {
        serial_puts("\e[2~");
    }
    else if (keycode == HID_KEY_PAUSE) {
        serial_puts("\e[3~");
    }
    else if (keycode == HID_KEY_PAGE_UP) {
        serial_puts("\e[5~");
    }
    else if (keycode == HID_KEY_PAGE_DOWN) {
        serial_puts("\e[6~");
    }
    else {
        serial_putc(ch);
    }
}

void term_key_pressed(uint8_t keycode, bool is_shift, bool is_ctrl) {
    for (int i = 0; i < MAX_PRESSED_KEYS; i++) {
        if (key_pressed_code[i] == 0) {
            key_pressed_code[i] = keycode;
            key_pressed_since[i] = time_us_32();
            key_is_ctrl = is_ctrl;
            key_is_shift = is_shift;
            term_key_sendcode(keycode, is_shift, is_ctrl);
            break;
        }
    }
}

void term_key_released(uint8_t keycode) {
    for (int i = 0; i < MAX_PRESSED_KEYS; i++) {
        if (key_pressed_code[i] == keycode) {
            key_pressed_code[i] = 0;
        }
    }
}

void term_update_screen() {
    // This function compares front buffer and back buffer for the difference.
    // It updates at most MAX_UPDATE char at a time and return.
    int update_count = 0;

    for (int y = 0; y < TERM_BUF_HEIGHT; y++) {
        for (int x = 0; x < TERM_WIDTH; x++) {
            char text = term_state_back->textmap[y][x];
            char color = term_state_back->colormap[y][x];
            char flag = term_state_back->flagmap[y][x];

            if ((term_state_front->textmap[y][x] != text) ||
                    (term_state_front->colormap[y][x] != color) ||
                    (term_state_front->flagmap[y][x] != flag)) {
                // Diff found
                term_state_front->textmap[y][x] = text;
                term_state_front->colormap[y][x] = color;
                term_state_front->flagmap[y][x] = flag;
                char fg = (uint8_t)color >> 4;
                char bg = color & 0xf;
                graph_put_char(x * 8, y * 16, text, fg, bg, flag);
                update_count ++;
                if (update_count > MAX_UPDATE)
                    return;
            }
        }
    }

    if ((term_state_back->x != term_state_front->x) || 
            (term_state_back->y != term_state_front->y)) {
        term_clear_cursor();
        term_state_front->x = term_state_back->x;
        term_state_front->y = term_state_back->y;
        term_disp_cursor();
    }

    if (term_state_back->y_offset != term_state_front->y_offset) {
        term_clear_cursor();
        term_state_front->y_offset = term_state_back->y_offset;
        //frame_scroll_lines = term_state_front->y_offset * 16;
    }

    term_state_dirty = false;
}

int term_printf(const char *format, ...) {
    char printf_buffer[256];

    int length = 0;

    va_list ap;
    va_start(ap, format);

    length = vsnprintf(printf_buffer, 256, format, ap);

    va_end(ap);

    term_process_string(printf_buffer);

    return length;
}

void term_init() {
    static repeating_timer_t timer;

    add_repeating_timer_ms(-100, term_timer_callback, NULL, &timer);
    gpio_init(25);
    gpio_set_dir(25, GPIO_OUT);

    term_full_reset();
    cursor_state = false;
    memset(term_state_front, 0, sizeof(*term_state_front));

    term_process_string("ELTerm 0.01\r\n");
#if 0
    uint64_t timediff = time_us_64();
    char fg = 1;
    char bg = COLOR_BLACK;
    for (int y = 0; y < 30; y++) {
        for (int x = 0; x < 80; x++) {
            graph_put_char(x * 8, y * 16, x+ 20, fg, bg);
        }
        fg++;
        if (fg==bg) fg++;
        if (fg==5) fg=7;
        if (fg==8) {
            fg = 0;
            bg++;
            if (bg==5) bg=7;
            if (bg==8) bg=0;
        };
    }
    for (int c = 0; c < 6; c++) {
        for (int y = 0; y < 30; y++) {
            for (int x = 0; x < 80; x++) {
                graph_fill_rect(x * 8, y * 16, (x + 1) * 8, (y + 1) * 16, c);
                //graph_put_char(x * 8, y * 16, x+ 20, fg, bg);
            }
        }
        //graph_fill_rect(0, 0, 640, 480, y);
    }
    timediff = time_us_64() - timediff;
    snprintf(debugmsg, MAX_DEBUG_LEN, "Rendered in %lld us", timediff);
    term_show_debug_message(debugmsg);

    while(1);
#endif
}

void term_loop() {
    static int timer_div = 0;
    char c;

    // Process timing related work
    if (timer_pending) {
        // Timer interval: 100ms
        timer_pending = false;

        timer_div++;
        if (timer_div == 5) {
            // Cursor update every 500ms
            cursor_state = !cursor_state;
            term_update_cursor();
            gpio_put(25, cursor_state);
            timer_div = 0;
        }

        // Key repeat
        for (int i = 0; i < MAX_PRESSED_KEYS; i++) {
            if ((key_pressed_code[i] != 0) &&
                    ((time_us_32() - key_pressed_since[i]) > 1000000)) {
                term_key_sendcode(key_pressed_code[i], key_is_shift, key_is_ctrl);
            }
        }
        
    }
    // Process all chars in the FIFO
    while (serial_getc(&c)) {
        term_process_char(c);
    }
    // Update up to one char on screen
    if (term_state_dirty) {
        term_update_screen();
    }
    if (frame_sync) {
        frame_sync = false;
        // Smooth scrolling
        uint32_t cur_scroll_lines = frame_scroll_lines;
        uint32_t new_scroll_lines;
        uint32_t target_scroll_lines = term_state_front->y_offset * 16;
        if (cur_scroll_lines != target_scroll_lines) {
            if (cur_scroll_lines < target_scroll_lines) {
                if (cur_scroll_lines < (target_scroll_lines - 16)) {
                    new_scroll_lines = target_scroll_lines - 16;
                }
                else {
                    new_scroll_lines = frame_scroll_lines + 1;
                }
            }
            else {
                if (cur_scroll_lines < (target_scroll_lines + SCR_BUF_HEIGHT - 16)) {
                    new_scroll_lines = target_scroll_lines + SCR_BUF_HEIGHT - 16;
                }
                else {
                    new_scroll_lines = frame_scroll_lines + 1;
                }
            }
            if (new_scroll_lines >= SCR_BUF_HEIGHT)
                new_scroll_lines -= SCR_BUF_HEIGHT;
            frame_scroll_lines = new_scroll_lines;
        }
    }
    // Poll USB
    usbhid_polling();
}
