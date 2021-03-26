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
#include <stdlib.h>
#include "pico/stdlib.h"
#include "el.h"
#include "graphics.h"
#include "terminal.h"
#include "serial.h"
#include "usbhid.h"

// Additional line for scrolling
#define TERM_WIDTH 80
#define TERM_HEIGHT 30
#define TERM_BUF_HEIGHT (TERM_HEIGHT+2)
#define MAX_DEBUG_LEN 107
char debugmsg[MAX_DEBUG_LEN];

typedef struct {
    char textmap[TERM_BUF_HEIGHT][TERM_WIDTH];
    char flagmap[TERM_BUF_HEIGHT][TERM_WIDTH];
    char colormap[TERM_BUF_HEIGHT][TERM_WIDTH];
    int x, y, y_offset;
} TERM_STATE;

typedef enum {
    ST_NORMAL,
    ST_ANSI_ESCAPE,
    ST_CSI_SEQ,
    ST_LSC_SEQ,
    ST_G0S_SEQ,
    ST_G1S_SEQ
} PARSER_STATE;

static TERM_STATE term_state_back;
static TERM_STATE term_state_front;
static bool term_state_dirty = false;

#define COLOR_BLACK (0)
#define COLOR_RED (1)
#define COLOR_GREEN (1)
#define COLOR_BROWN (1)
#define COLOR_BLUE (1)
#define COLOR_MAGENTA (2)
#define COLOR_CYAN (2)
#define COLOR_WHITE (7)
#define COLOR_GRAY (3)
#define COLOR_BRIGHT_RED (3)
#define COLOR_BRIGHT_GREEN (3)
#define COLOR_BRIGHT_YELLOW (4)
#define COLOR_BRIGHT_BLUE (3)
#define COLOR_BRIGHT_MAGENTA (4)
#define COLOR_BRIGHT_CYAN (4)
#define COLOR_BRIGHT_WHITE (7)

#define DEFAULT_COLOR ((COLOR_WHITE << 4) | (COLOR_BLACK))

#define MAX_UPDATE (80 * 10)

static bool cursor_state = false;
static volatile bool timer_pending = false;
static char current_color = DEFAULT_COLOR;
static char current_flag = 0;
// Saved cursor for DECSC and DECRC
static int saved_x, saved_y;
static char saved_color, saved_flag;
// Modes
static bool mode_auto_warp = true;

void term_scroll() {
    // TODO
    term_state_back.y++;
    if (term_state_back.y >= TERM_HEIGHT) {
        // Scroll
        term_state_back.y --;
        term_state_back.y_offset ++;
        if (term_state_back.y_offset >= TERM_BUF_HEIGHT)
            term_state_back.y_offset -= TERM_BUF_HEIGHT;
        int cy = term_state_back.y + term_state_back.y_offset;
        if (cy >= TERM_BUF_HEIGHT) cy -= TERM_BUF_HEIGHT;
        for (int x = 0; x < TERM_WIDTH; x++) {
            term_state_back.textmap[cy][x] = ' ';
        }
    }
    term_state_dirty = true;
}

void term_cursor_backward() {
    if (term_state_back.x > 0) {
        term_state_back.x--;
    }
    else {
        if (term_state_back.y > 0) {
            term_state_back.x = TERM_WIDTH - 1;
            term_state_back.y--;
        }
    }
    term_state_dirty = true;
}

void term_cursor_forward() {
    term_state_back.x++;
    if (term_state_back.x >= TERM_WIDTH) {
        if (mode_auto_warp) {
            term_state_back.x = 0;
            term_scroll();
        }
        else {
            term_state_back.x = TERM_WIDTH - 1;
        }
    }
    term_state_dirty = true;
}

void term_cursor_set(int x, int y) {
    term_state_back.x = x;
    term_state_back.y = y;
    term_state_dirty = true;
}

void term_clear_cursor() {
    int x = term_state_front.x;
    int y = term_state_front.y + term_state_front.y_offset;
    if (y >= TERM_BUF_HEIGHT) y -= TERM_BUF_HEIGHT;
    char text = term_state_front.textmap[y][x];
    char color = term_state_front.colormap[y][x];
    char fg = (uint8_t)color >> 4;
    char bg = color & 0xf;
    char flag = term_state_front.flagmap[y][x];
    graph_put_char(x * 8, y * 16, text, fg, bg, flag);
}

void term_disp_cursor() {
    int x = term_state_front.x;
    int y = term_state_front.y + term_state_front.y_offset;
    if (y >= TERM_BUF_HEIGHT) y -= TERM_BUF_HEIGHT;
    graph_fill_rect(x * 8, y * 16, (x + 1) * 8, (y + 1) * 16, COLOR_WHITE);
}

void term_update_cursor() {
    if (cursor_state) {
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

static void term_dec_modeset(int mode, bool enable) {
    if (mode == 7) {
        mode_auto_warp = enable;
    }
    else if (mode == 2004) {
        // Bracketed paste mode. Ignore
    }
    else {
        snprintf(debugmsg, MAX_DEBUG_LEN, "Unsupported DEC mode: %d", mode);
        term_show_debug_message(debugmsg);
    }
}

static void term_modeset(int mode, bool enable) {
        snprintf(debugmsg, MAX_DEBUG_LEN, "Unsupported mode: %d", mode);
        term_show_debug_message(debugmsg);
}

static void term_put_char(int x, int y, char c) {
    int ay = y + term_state_back.y_offset;
    if (ay >= TERM_BUF_HEIGHT) ay -= TERM_BUF_HEIGHT;
    term_state_back.textmap[ay][x] = c;
    term_state_back.flagmap[ay][x] = current_flag;
    term_state_back.colormap[ay][x] = current_color;
    term_state_dirty = true;
}

static void term_set_fg(char fg) {
    current_color &= 0x0f;
    current_color |= (fg << 4);
}

static void term_set_bg(char bg) {
    current_color &= 0xf0;
    current_color |= bg;
}

static void term_cursor_down(int lines) {
    int x = term_state_back.x;
    int y = term_state_back.y;
    y += lines;
    if (y >= TERM_HEIGHT) y = TERM_HEIGHT - 1;
    term_cursor_set(x, y);
}

static void term_cursor_up(int lines) {
    int x = term_state_back.x;
    int y = term_state_back.y;
    y -= lines;
    if (y < 0) y = 0;
    term_cursor_set(x, y);
}

static void term_report_dev_attributes() {
    serial_puts("\e[?1;0c");
}

static void term_report_cursor() {
    char str[20];
    snprintf(str, 20, "\e[%d;%dR", term_state_back.y + 1, term_state_back.x + 1);
    serial_puts(str);
}

static void term_reset() {
    saved_x = 0;
    saved_y = 0;
    saved_color = DEFAULT_COLOR;
    saved_flag = 0;
    current_color = DEFAULT_COLOR;
    current_flag = 0;
    mode_auto_warp = true;
    memset(&term_state_back, 0, sizeof(term_state_back));
}

void term_process_char(char c) {
    // States
    static char csi[5];
    static int csi_codes[5];
    static int arg_counter = 0;
    static int chr_counter = 0;
    static PARSER_STATE state = ST_NORMAL;
    static bool dec_set = false;
    int x = term_state_back.x;
    int y = term_state_back.y;

    if (state == ST_NORMAL) {
        if ((c == 0x08) || (c == 0x7f)) {
            // BS
            term_cursor_backward();
        }
        else if (c == 0x0d) {
            // CR
            term_cursor_set(0, term_state_back.y);
            //term_scroll();
        }
        else if ((c == 0x0a) || (c == 0x0b) || (c == 0x0c)) {
            // LF
            term_scroll();
        }
        else if (c == 0x09) {
            // Tab
            x += 7;
            x &= ~3;
            term_cursor_set(x, y);
            if (x >= TERM_WIDTH) {
                term_cursor_set(0, y);
                term_scroll();
            }
        }
        else if (c == 0x07) {
            // Bell
        }
        else if (c == 0x1b) {
            state = ST_ANSI_ESCAPE;
        }
        else {
            term_put_char(x, y, c);
            term_cursor_forward();
        }
    }
    else if (state == ST_ANSI_ESCAPE) {
        if (c == '[') {
            state = ST_CSI_SEQ;
            dec_set = false;
            arg_counter = 0;
            chr_counter = 0;
        }
        else if (c == '#') {
            state = ST_LSC_SEQ;
        }
        else if (c == '(') {
            state = ST_G0S_SEQ;
        }
        else if (c == ')') {
            state = ST_G1S_SEQ;
        }
        else if (c == '7') {
            // DECSC: Save Cursor
            saved_x = term_state_back.x;
            saved_y = term_state_back.y;
            saved_color = current_color;
            saved_flag = current_flag;
            state = ST_NORMAL;
        }
        else if (c == '8') {
            // DECRC: Restore Cursor
            term_state_back.x = saved_x;
            term_state_back.y = saved_y;
            current_color = saved_color;
            current_flag = saved_flag;
            state = ST_NORMAL;
        }
        else if (c == 'D') {
            // IND: Index
            term_cursor_down(1);
            state = ST_NORMAL;
        }
        else if (c == 'E') {
            // NEL: Next Line
            term_scroll();
            state = ST_NORMAL;
        }
        else if (c == 'M') {
            // RI: Reverse Index
            term_cursor_up(1);
            state = ST_NORMAL;
        }
        else if (c == 'Z') {
            // DECID: Identify
            term_report_dev_attributes();
            state = ST_NORMAL;
        }
        else if (c == 'c') {
            // RIS: Reset to Initial State
            term_reset();
            state = ST_NORMAL;
        }
        else {
            snprintf(debugmsg, MAX_DEBUG_LEN, "Unsupported escape sequence: %c (%d)", c, c);
            term_show_debug_message(debugmsg);
            state = ST_NORMAL;
        }
    }
    else if (state == ST_CSI_SEQ) {
        if ((c >= 0x30) && (c <= 0x39)) {
            csi[chr_counter++] = c;
            if (chr_counter > 4) {
                term_show_debug_message("CSI sequence argument too long");
                state = ST_NORMAL;
            }
        }
        else {
            csi[chr_counter] = '\0';
            if (chr_counter != 0) {
                csi_codes[arg_counter++] = atoi(csi);
                chr_counter = 0;
            }
            if (arg_counter > 4) {
                term_show_debug_message("Too many arguments in one CSI sequence");
                state = ST_NORMAL;
                return;
            }
            
            if (c == 'm') {
                // SGR sequcne
                if (arg_counter == 0) {
                    csi_codes[0] = 0;
                    arg_counter = 1;
                }
                for (int i = 0; i < arg_counter; i++) {
                    switch (csi_codes[i]) {
                    case 0: // Reset
                        current_flag = 0;
                        current_color = DEFAULT_COLOR;
                        break;
                    case 1: // Bold
                        current_flag |= FLAG_BOLD; break;
                    case 3: // Italic
                        current_flag |= FLAG_ITALIC; break;
                    case 4: // Underline
                        current_flag |= FLAG_UNDERLINE; break;
                    case 5: // Slow blink
                        current_flag |= FLAG_SLOWBLINK; break;
                    case 7:
                        current_flag |= FLAG_INVERT; break;
                    case 9: // Croseed out
                        current_flag |= FLAG_STHROUGH; break;
                    case 22: // Bold off
                        current_flag &= ~FLAG_BOLD; break;
                    case 23: // Italic off
                        current_flag &= ~FLAG_ITALIC; break;
                    case 24: // Underline off
                        current_flag &= ~FLAG_UNDERLINE; break;
                    case 25: // Blink off
                        current_flag &= ~FLAG_SLOWBLINK; break;
                    case 26: // Crossed out off
                        current_flag &= ~FLAG_STHROUGH; break;
                    case 27:
                        current_flag &= ~FLAG_INVERT; break;
                    case 30: term_set_fg(COLOR_BLACK); break;
                    case 31: term_set_fg(COLOR_RED); break;
                    case 32: term_set_fg(COLOR_GREEN); break;
                    case 33: term_set_fg(COLOR_BROWN); break;
                    case 34: term_set_fg(COLOR_BLUE); break;
                    case 35: term_set_fg(COLOR_MAGENTA); break;
                    case 36: term_set_fg(COLOR_CYAN); break;
                    case 37: term_set_fg(COLOR_WHITE); break;
                    case 90: term_set_fg(COLOR_GRAY); break;
                    case 91: term_set_fg(COLOR_BRIGHT_RED); break;
                    case 92: term_set_fg(COLOR_BRIGHT_GREEN); break;
                    case 93: term_set_fg(COLOR_BRIGHT_YELLOW); break;
                    case 94: term_set_fg(COLOR_BRIGHT_BLUE); break;
                    case 95: term_set_fg(COLOR_BRIGHT_MAGENTA); break;
                    case 96: term_set_fg(COLOR_BRIGHT_CYAN); break;
                    case 97: term_set_fg(COLOR_BRIGHT_WHITE); break;
                    case 40: term_set_bg(COLOR_BLACK); break;
                    case 41: term_set_bg(COLOR_RED); break;
                    case 42: term_set_bg(COLOR_GREEN); break;
                    case 43: term_set_bg(COLOR_BROWN); break;
                    case 44: term_set_bg(COLOR_BLUE); break;
                    case 45: term_set_bg(COLOR_MAGENTA); break;
                    case 46: term_set_bg(COLOR_CYAN); break;
                    case 47: term_set_bg(COLOR_WHITE); break;
                    case 100:term_set_bg(COLOR_GRAY); break;
                    case 101:term_set_bg(COLOR_BRIGHT_RED); break;
                    case 102:term_set_bg(COLOR_BRIGHT_GREEN); break;
                    case 103:term_set_bg(COLOR_BRIGHT_YELLOW); break;
                    case 104:term_set_bg(COLOR_BRIGHT_BLUE); break;
                    case 105:term_set_bg(COLOR_BRIGHT_MAGENTA); break;
                    case 106:term_set_bg(COLOR_BRIGHT_CYAN); break;
                    case 107:term_set_bg(COLOR_BRIGHT_WHITE); break;
                    default:
                        snprintf(debugmsg, MAX_DEBUG_LEN, "Unsupported SGR code: %d", csi_codes[i]);
                        term_show_debug_message(debugmsg);
                    }
                }
                state = ST_NORMAL;
            }
            else if (c == '?') {
                dec_set = true;
            }
            else if (c == 'A') {
                // CUU: Cursor Up
                if (arg_counter == 0) csi_codes[0] = 1;
                term_cursor_up(csi_codes[0]);
                state = ST_NORMAL;
            }
            else if (c == 'B') {
                // CUD: Cursor Down
                if (arg_counter == 0) csi_codes[0] = 1;
                term_cursor_down(csi_codes[0]);
                state = ST_NORMAL;
            }
            else if (c == 'C') {
                // CUF: Cursor Forward
                if (arg_counter == 0) csi_codes[0] = 1;
                x += csi_codes[0];
                if (x >= TERM_WIDTH) x = TERM_WIDTH - 1;
                term_cursor_set(x, y);
                state = ST_NORMAL;
            }
            else if (c == 'D') {
                // CUB: Cursor Back
                if (arg_counter == 0) csi_codes[0] = 1;
                x -= csi_codes[0];
                if (x < 0) x = 0;
                term_cursor_set(x, y);
                state = ST_NORMAL;
            }
            /*else if (c == 'G') {
                if (arg_counter == 0) csi_codes[0] = 1;
                x = csi_codes[0] - 1;
                term_cursor_set(x, y);
                state = ST_NORMAL;
            }
            else if (c == 'd') {
                if (arg_counter == 0) csi_codes[0] = 1;
                y = csi_codes[0] - 1;
                term_cursor_set(x, y);
                state = ST_NORMAL;
            }*/
            else if ((c == 'H') || (c == 'f')) {
                // CUP: Cursor Position
                // HVP: Horizontal Vertical Position
                if (arg_counter < 2)
                    csi_codes[1] = 1;
                if (arg_counter < 1)
                    csi_codes[0] = 1;
                y = csi_codes[0] - 1;
                x = csi_codes[1] - 1;
                if (x < 0) x = 0;
                if (x >= TERM_WIDTH) x= TERM_WIDTH - 1;
                if (y < 0) y = 0;
                if (y >= TERM_HEIGHT) y = TERM_HEIGHT - 1;
                term_cursor_set(x, y);
                state = ST_NORMAL;
            }
            else if (c == 'K') {
                // EL: Erase in Line
                if (arg_counter == 0) csi_codes[0] = 0;
                if (csi_codes[0] == 0) {
                    for (; x < TERM_WIDTH; x++) {
                        term_put_char(x, y, ' ');
                    }
                }
                else if (csi_codes[0] == 1) {
                    for (int xx = 0; xx <= x; xx++) {
                        term_put_char(x, y, ' ');
                    }
                }
                else {
                    for (x = 0; x < TERM_WIDTH; x++) {
                        term_put_char(x, y, ' ');
                    }
                }
                state = ST_NORMAL;
            }
            else if (c == 'J') {
                // ED: Erase in Display
                if (arg_counter == 0) csi_codes[0] = 0;
                if (csi_codes[0] == 0) {
                    for (int xx = x; xx < TERM_WIDTH; xx++) {
                        term_put_char(xx, y, ' ');
                    }
                    for (int yy = y + 1; yy < TERM_HEIGHT; yy++) {
                        for (int xx = 0; xx < TERM_WIDTH; xx++) {
                            term_put_char(xx, yy, ' ');
                        }
                    }
                }
                else if (csi_codes[0] == 1) {
                    for (int yy = 0; yy < y; yy++) {
                        for (int xx = 0; xx < TERM_WIDTH; xx++) {
                            term_put_char(xx, yy, ' ');
                        }
                    }
                    for (int xx = 0; xx <= x; xx++) {
                        term_put_char(xx, y, ' ');
                    }
                }
                else {
                    for (y = 0; y < TERM_HEIGHT; y++) {
                        for (x = 0; x < TERM_WIDTH; x++) {
                            term_put_char(x, y, ' ');
                        }
                    }
                }
                state = ST_NORMAL;
            }
            else if (c == 'n') {
                // DSR: Device Status Report
                if (csi_codes[0] == 5) {
                    serial_puts("\e[0n"); // Ready
                }
                else if (csi_codes[0] == 6) {
                    term_report_cursor();
                }
                state = ST_NORMAL;
            }
            else if (c == 'c') {
                // DA: Device Attributes
                term_report_dev_attributes();
                state = ST_NORMAL;
            }
            else if (c == '@') {
                // ICH: Insert Character
                if (arg_counter == 1) {
                    int shift = csi_codes[0];
                    for (int xx = TERM_WIDTH; xx >= x + shift; xx++) {
                        term_state_back.textmap[y][xx] = term_state_back.textmap[y][xx - shift];
                        term_state_back.colormap[y][xx] = term_state_back.colormap[y][xx - shift];
                        term_state_back.flagmap[y][xx] = term_state_back.flagmap[y][xx - shift];
                    }
                    for (int xx = x; xx < x + shift; xx++) {
                        if (xx >= TERM_WIDTH)
                            break;
                        term_state_back.textmap[y][xx] = ' ';
                        term_state_back.colormap[y][xx] = current_color;
                        term_state_back.flagmap[y][xx] = current_flag;
                    }
                    term_state_dirty = true;
                }
                state = ST_NORMAL;
            }
            else if (c == 'P') {
                // DCH: Delete Character
                if (arg_counter == 1) {
                    int shift = csi_codes[0];
                    if (shift > (TERM_WIDTH - x))
                        shift = TERM_WIDTH - x;
                    for (int xx = x; xx < (TERM_WIDTH - shift); xx++) {
                        term_state_back.textmap[y][xx] = term_state_back.textmap[y][xx + shift];
                        term_state_back.colormap[y][xx] = term_state_back.colormap[y][xx + shift];
                        term_state_back.flagmap[y][xx] = term_state_back.flagmap[y][xx + shift];
                    }
                    for (int xx = TERM_WIDTH - shift; xx < TERM_WIDTH; xx++) {
                        term_state_back.textmap[y][xx] = ' ';
                        term_state_back.colormap[y][xx] = current_color;
                        term_state_back.flagmap[y][xx] = current_flag;
                    }
                    term_state_dirty = true;
                }
                state = ST_NORMAL;
            }
            else if (c == 'X') {
                // ECH: Erase Character
                if (arg_counter == 1) {
                    int shift = csi_codes[0];
                    if (shift > (TERM_WIDTH - x))
                        shift = TERM_WIDTH - x;
                    for (int xx = x; xx < x + shift; xx++) {
                        term_state_back.textmap[y][xx] = ' ';
                        term_state_back.colormap[y][xx] = current_color;
                        term_state_back.flagmap[y][xx] = current_flag;
                    }
                    term_state_dirty = true;
                }
                state = ST_NORMAL;
            }
            else if (c == 'r') {
                // DECSTBM: Set Scrolling Region
                // Scrolling is not supported, ignore
                state = ST_NORMAL;
            }
            else if (c == 'h') {
                // Mode setting
                for (int i = 0; i < arg_counter; i++) {
                    if (dec_set)
                        term_dec_modeset(csi_codes[i], true);
                    else
                        term_modeset(csi_codes[i], true);
                }
                state = ST_NORMAL;
            }
            else if (c == 'l') {
                // Mode setting
                for (int i = 0; i < arg_counter; i++) {
                    if (dec_set)
                        term_dec_modeset(csi_codes[i], false);
                    else
                        term_modeset(csi_codes[i], false);
                }
                state = ST_NORMAL;
            }
            else if (c == ';') {
                // not end yet. continue
            }
            else {
                snprintf(debugmsg, MAX_DEBUG_LEN, "Unsupported CSI seq: %c (%d)", c, c);
                term_show_debug_message(debugmsg);
                state = ST_NORMAL;
            }

            /*if (state == ST_NORMAL) {
                printf("CSI");
                for (int i = 0; i < arg_counter; i++) {
                    printf("%d ", csi_codes[i]);
                }
                printf("%c\n", c);
            }*/
        }
    }
    else if (state == ST_LSC_SEQ) {
        // Silently ignore LSC sequence
        state = ST_NORMAL;
    }
    else if (state == ST_G0S_SEQ) {
        // Silently ignore G0 SCS
        state = ST_NORMAL;
    }
    else if (state == ST_G1S_SEQ) {
        // Silently ignore G1 SCS
        state = ST_NORMAL;
    }
}

void term_process_string(char *str) {
    char c;
    while (c = *str++) {
        term_process_char(c);
    }
}

void term_update_screen() {
    // This function compares front buffer and back buffer for the difference.
    // It updates at most MAX_UPDATE char at a time and return.
    int update_count = 0;

    for (int y = 0; y < TERM_BUF_HEIGHT; y++) {
        for (int x = 0; x < TERM_WIDTH; x++) {
            char text = term_state_back.textmap[y][x];
            char color = term_state_back.colormap[y][x];
            char flag = term_state_back.flagmap[y][x];

            if ((term_state_front.textmap[y][x] != text) ||
                    (term_state_front.colormap[y][x] != color) ||
                    (term_state_front.flagmap[y][x] != flag)) {
                // Diff found
                term_state_front.textmap[y][x] = text;
                term_state_front.colormap[y][x] = color;
                term_state_front.flagmap[y][x] = flag;
                char fg = (uint8_t)color >> 4;
                char bg = color & 0xf;
                graph_put_char(x * 8, y * 16, text, fg, bg, flag);
                update_count ++;
                if (update_count > MAX_UPDATE)
                    return;
            }
        }
    }

    if ((term_state_back.x != term_state_front.x) || 
            (term_state_back.y != term_state_front.y)) {
        term_clear_cursor();
        term_state_front.x = term_state_back.x;
        term_state_front.y = term_state_back.y;
        term_disp_cursor();
    }

    if (term_state_back.y_offset != term_state_front.y_offset) {
        term_clear_cursor();
        term_state_front.y_offset = term_state_back.y_offset;
        //frame_scroll_lines = term_state_front.y_offset * 16;
    }

    term_state_dirty = false;
}

void term_main() {
    char c;

    struct repeating_timer timer;

    add_repeating_timer_ms(-500, term_timer_callback, NULL, &timer);
    gpio_init(25);
    gpio_set_dir(25, GPIO_OUT);

    memset(&term_state_back, 0, sizeof(term_state_back));
    memset(&term_state_front, 0, sizeof(term_state_front));

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

    while (1) {
        // Process timing related work
        if (timer_pending) {
            timer_pending = false;
            cursor_state = !cursor_state;
            term_update_cursor();
            gpio_put(25, cursor_state);
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
            uint32_t target_scroll_lines = term_state_front.y_offset * 16;
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
}
