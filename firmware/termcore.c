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

// Reference: https://www.xfree86.org/current/ctlseqs.html
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "termcore.h"
#include "graphics.h"

// Private Definitions
typedef enum {
    ST_NORMAL,
    ST_ANSI_ESCAPE,
    ST_CSI_SEQ,
    ST_LSC_SEQ,
    ST_G0S_SEQ,
    ST_G1S_SEQ,
    ST_OSC_SEQ,
    ST_OSC_PAR,
} PARSER_STATE;

static TERM_STATE term_state_back_main;
static TERM_STATE term_state_back_alternate;

TERM_STATE *term_state_back = &term_state_back_main;
bool term_state_dirty = false;
static char current_color = DEFAULT_COLOR;
static char current_flag = 0;
// Saved cursor for DECSC and DECRC
static int saved_x, saved_y;
// Saved cursor for alternative buffer
static int alt_x, alt_y;
static char saved_color, saved_flag;
// Modes
static bool mode_auto_warp = true;
bool mode_app_keypad = false;
bool mode_app_cursor = false;
bool mode_cursor_blinking = true;
bool mode_show_cursor = true;
static bool mode_insert = false;
static bool mode_auto_newline = false;
char last_graph_char = '\0';

static bool pending_wrap = false;

static void term_scroll() {
    term_state_back->y++;
    if (term_state_back->y >= TERM_HEIGHT) {
        // Scroll
        term_state_back->y --;
        term_state_back->y_offset ++;
        if (term_state_back->y_offset >= TERM_BUF_HEIGHT)
            term_state_back->y_offset -= TERM_BUF_HEIGHT;
        int cy = term_state_back->y + term_state_back->y_offset;
        if (cy >= TERM_BUF_HEIGHT) cy -= TERM_BUF_HEIGHT;
        for (int x = 0; x < TERM_WIDTH; x++) {
            term_state_back->textmap[cy][x] = ' ';
        }
    }
    pending_wrap = false;
    term_state_dirty = true;
}

static void term_cursor_backward() {
    pending_wrap = false;
    if (term_state_back->x > 0) {
        term_state_back->x--;
    }
    else {
        if (term_state_back->y > 0) {
            term_state_back->x = TERM_WIDTH - 1;
            term_state_back->y--;
        }
    }
    term_state_dirty = true;
}

static void term_cursor_check() {
    if ((mode_auto_warp) && (pending_wrap)) {
        term_state_back->x = 0;
        term_scroll();
    }
}

static void term_cursor_forward() {
    term_state_back->x++;
    if (term_state_back->x >= TERM_WIDTH) {
        term_state_back->x = TERM_WIDTH - 1;
        if (mode_auto_warp) {
            // only advance when the next char is entered
            pending_wrap = true;
        }
    }
    term_state_dirty = true;
}

static void term_cursor_set(int x, int y) {
    // Forced update clears pending wrap
    pending_wrap = false;
    term_state_back->x = x;
    term_state_back->y = y;
    term_state_dirty = true;
}

// DECSET
static void term_dec_modeset(int mode, bool enable) {
    if (mode == 1) {
        mode_app_cursor = enable;
    }
    else if (mode == 7) {
        mode_auto_warp = enable;
    }
    else if (mode == 12) {
        mode_cursor_blinking = enable;
    }
    else if (mode == 25) {
        mode_show_cursor = enable;
    }
    else if ((mode == 47) || (mode == 1047)) {
        // Use Alternate Screen Buffer
        if (enable)
            term_state_back = &term_state_back_alternate;
        else
            term_state_back = &term_state_back_main;
    }
    else if (mode == 1048) {
        if (enable) {
            alt_x = term_state_back->x;
            alt_y = term_state_back->y;
        }
        else {
            term_state_back->x = alt_x;
            term_state_back->y = alt_y;
        }
    }
    else if (mode == 1049) {
        // Use Alternate Screen Buffer with clearing, save cursors
        if (enable) {
            term_state_back = &term_state_back_alternate;
            alt_x = term_state_back->x;
            alt_y = term_state_back->y;
        }
        else {
            term_state_back = &term_state_back_main;
            term_state_back->x = alt_x;
            term_state_back->y = alt_y;
        }   
        memset(term_state_back, 0, sizeof(*term_state_back));
        term_state_dirty = true;
    }
    else if (mode == 2004) {
        // Bracketed paste mode. Ignore
    }
    else {
        fprintf(stderr, "Unsupported DEC mode: %d", mode);
    }
}

// SM
static void term_modeset(int mode, bool enable) {
    if (mode == 4) {
        mode_insert = enable;
    }
    else if (mode == 20) {
        mode_auto_newline = enable;
        printf("Auto new line set to %d\n", enable);
    }
    else {
        fprintf(stderr, "Unsupported mode: %d", mode);
    }
}

static void term_put_char(int x, int y, char c) {
    int ay = y + term_state_back->y_offset;
    if (ay >= TERM_BUF_HEIGHT) ay -= TERM_BUF_HEIGHT;
    term_state_back->textmap[ay][x] = c;
    term_state_back->flagmap[ay][x] = current_flag;
    term_state_back->colormap[ay][x] = current_color;
    term_state_dirty = true;
    //printf("putc %d %d = %c\n", x, y, c);
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
    int x = term_state_back->x;
    int y = term_state_back->y;
    y += lines;
    if (y >= TERM_HEIGHT) y = TERM_HEIGHT - 1;
    term_cursor_set(x, y);
}

static void term_cursor_up(int lines) {
    int x = term_state_back->x;
    int y = term_state_back->y;
    y -= lines;
    if (y < 0) y = 0;
    term_cursor_set(x, y);
}

static void term_forward_tab() {
    term_cursor_check();
    int x, y;
    x = term_state_back->x + 8;
    x &= ~7;
    y = term_state_back->y;
    
    if (x >= TERM_WIDTH) {
        for (int i = term_state_back->x; i < TERM_WIDTH; i++) {
            term_put_char(i, y, 0x20);
        }
        if (mode_auto_warp) {
            pending_wrap = true;
        }
        term_cursor_set(TERM_WIDTH - 1, y);
    }
    else {
        for (int i = term_state_back->x; i < x; i++) {
            term_put_char(i, y, 0x20);
        }
        term_cursor_set(x, y);
    }
}

static void term_backward_tab() {
    int x, y;
    x = term_state_back->x - 1;
    x &= ~7;
    y = term_state_back->y;
    if (x < 0) x = 0;
    
    term_cursor_set(x, y);
}

static void term_shift_right(int shift) {
    int x = term_state_back->x;
    int y = term_state_back->y;
    for (int xx = TERM_WIDTH - 1; xx >= x + shift; xx--) {
        term_state_back->textmap[y][xx] = term_state_back->textmap[y][xx - shift];
        term_state_back->colormap[y][xx] = term_state_back->colormap[y][xx - shift];
        term_state_back->flagmap[y][xx] = term_state_back->flagmap[y][xx - shift];
    }
    for (int xx = x; xx < x + shift; xx++) {
        if (xx >= TERM_WIDTH)
            break;
        term_state_back->textmap[y][xx] = ' ';
        term_state_back->colormap[y][xx] = current_color;
        term_state_back->flagmap[y][xx] = current_flag;
    }
    term_state_dirty = true;
}

static void term_shift_down(int shift) {
    int x = term_state_back->x;
    int y = term_state_back->y;
    for (int yy = TERM_BUF_HEIGHT - 1; yy >= y + shift; yy--){
        memcpy(term_state_back->textmap[yy], term_state_back->textmap[yy - shift], TERM_WIDTH);
        memcpy(term_state_back->colormap[yy], term_state_back->colormap[yy - shift], TERM_WIDTH);
        memcpy(term_state_back->flagmap[yy], term_state_back->flagmap[yy - shift], TERM_WIDTH);
    }
    for (int yy = y; yy < y + shift; yy++) {
        if (yy >= TERM_BUF_HEIGHT)
            break;
        memset(term_state_back->textmap[yy], ' ', TERM_WIDTH);
        memset(term_state_back->colormap[yy], current_color, TERM_WIDTH);
        memset(term_state_back->flagmap[yy], current_flag, TERM_WIDTH);
    }
    term_state_dirty = true;
}

static void term_shift_up(int shift) {
    int x = term_state_back->x;
    int y = term_state_back->y;
    for (int yy = y; yy < TERM_BUF_HEIGHT - shift; yy++){
        memcpy(term_state_back->textmap[yy], term_state_back->textmap[yy + shift], TERM_WIDTH);
        memcpy(term_state_back->colormap[yy], term_state_back->colormap[yy + shift], TERM_WIDTH);
        memcpy(term_state_back->flagmap[yy], term_state_back->flagmap[yy + shift], TERM_WIDTH);
    }
    for (int yy = TERM_BUF_HEIGHT - shift; yy < TERM_BUF_HEIGHT; yy++) {
        memset(term_state_back->textmap[yy], ' ', TERM_WIDTH);
        memset(term_state_back->colormap[yy], current_color, TERM_WIDTH);
        memset(term_state_back->flagmap[yy], current_flag, TERM_WIDTH);
    }
    term_state_dirty = true;
}

static void term_report_dev_attributes() {
    serial_puts("\e[?60;1;2;6;8;9;15;c");
}

static void term_report_cursor(bool dec_mode) {
    char str[20];
    int reportX, reportY;
    snprintf(str, 20, (dec_mode) ? "\e?[%d;%dR" : "\e[%d;%dR",
            term_state_back->y + 1, term_state_back->x + 1);
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
    mode_app_keypad = false;
    mode_app_cursor = false;
    mode_cursor_blinking = true;
    mode_show_cursor = true;
    mode_insert = false;
    mode_auto_newline = false;
    pending_wrap = false;
    last_graph_char = '\0';
    term_state_back = &term_state_back_main;
    memset(term_state_back, 0, sizeof(*term_state_back));
}

void term_process_char(uint8_t c) {
    // States
    static char csi[5];
    static int csi_codes[5];
    static int arg_counter = 0;
    static int chr_counter = 0;
    static int osc_type = 0;
    static PARSER_STATE state = ST_NORMAL;
    static bool dec_set = false;

    // ANSI behavior
    //term_cursor_check(); // force flush?

    int x = term_state_back->x;
    int y = term_state_back->y;

    //printf("Processing char %c at %d, %d\n", c, x, y);

    if (state == ST_NORMAL) {
        if ((c == 0x08) || (c == 0x7f)) {
            // BS
            term_cursor_backward();
        }
        else if (c == 0x0d) {
            // CR
            term_cursor_set(0, term_state_back->y);
            if (mode_auto_newline)
                term_scroll();
        }
        else if ((c == 0x0a) || (c == 0x0b) || (c == 0x0c)) {
            // LF
            term_scroll();
            term_cursor_set(0, term_state_back->y);
        }
        else if (c == 0x09) {
            // Tab
            term_forward_tab();
        }
        else if (c == 0x07) {
            // Bell
        }
        else if (c == 0x1b) {
            state = ST_ANSI_ESCAPE;
        }
        else if (c == 0xff) {
            fprintf(stderr, "IAC?\n");
        }
        else {
            last_graph_char = c;
            if (mode_insert) {
                term_cursor_check();
                term_shift_right(1);
                term_put_char(term_state_back->x, term_state_back->y, c);
                term_cursor_forward();
            }
            else {
                term_cursor_check();
                term_put_char(term_state_back->x, term_state_back->y, c);
                term_cursor_forward();
            }
            
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
        else if (c == ']') {
            state = ST_OSC_SEQ;
            chr_counter = 0;
        }
        else if (c == '7') {
            // DECSC: Save Cursor
            saved_x = term_state_back->x;
            saved_y = term_state_back->y;
            saved_color = current_color;
            saved_flag = current_flag;
            state = ST_NORMAL;
        }
        else if (c == '8') {
            // DECRC: Restore Cursor
            term_state_back->x = saved_x;
            term_state_back->y = saved_y;
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
            term_cursor_check();
            term_scroll();
            term_cursor_set(0, term_state_back->y);
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
        else if (c == '=') {
            // DECPAM: Application Keypad
            mode_app_keypad = true;
            state = ST_NORMAL;
        }
        else if (c == '>') {
            // DECPNM: Normal Keypad
            mode_app_keypad = false;
            state = ST_NORMAL;
        }
        else {
            fprintf(stderr, "Unsupported escape sequence: %c (%d)", c, c);
            state = ST_NORMAL;
        }
    }
    else if (state == ST_CSI_SEQ) {
        if ((c >= 0x30) && (c <= 0x39)) {
            csi[chr_counter++] = c;
            if (chr_counter > 4) {
                fprintf(stderr, "CSI sequence argument too long");
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
                fprintf(stderr, "Too many arguments in one CSI sequence");
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
                    case 10: // Default font, ignored
                        break;
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
                    case 39: term_set_fg(COLOR_WHITE); break;
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
                    case 49: term_set_bg(COLOR_BLACK); break;
                    case 100:term_set_bg(COLOR_GRAY); break;
                    case 101:term_set_bg(COLOR_BRIGHT_RED); break;
                    case 102:term_set_bg(COLOR_BRIGHT_GREEN); break;
                    case 103:term_set_bg(COLOR_BRIGHT_YELLOW); break;
                    case 104:term_set_bg(COLOR_BRIGHT_BLUE); break;
                    case 105:term_set_bg(COLOR_BRIGHT_MAGENTA); break;
                    case 106:term_set_bg(COLOR_BRIGHT_CYAN); break;
                    case 107:term_set_bg(COLOR_BRIGHT_WHITE); break;
                    default:
                        fprintf(stderr, "Unsupported SGR code: %d", csi_codes[i]);
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
            else if (c == 'E') {
                // CNL: next line
                if (arg_counter == 0) csi_codes[0] = 1;
                term_cursor_down(csi_codes[0]);
                term_cursor_set(0, term_state_back->y);
                state = ST_NORMAL;
            }
            else if (c == 'F') {
                // CPL: previous line
                if (arg_counter == 0) csi_codes[0] = 1;
                term_cursor_up(csi_codes[0]);
                term_cursor_set(0, term_state_back->y);
                state = ST_NORMAL;
            }
            else if ((c == 'G') || (c == '`')) {
                // CHA: Cursor Character Absolute
                // HPA: Character Position Absolute
                if (arg_counter == 0) csi_codes[0] = 1;
                x = csi_codes[0] - 1;
                term_cursor_set(x, y);
                state = ST_NORMAL;
            }
            else if (c == 'I') {
                // CHT
                if (arg_counter == 0) csi_codes[0] = 1;
                for (int i = 0; i < csi_codes[0]; i++) {
                    term_forward_tab();
                }
                state = ST_NORMAL;
            }
            else if (c == 'd') {
                // VPA
                if (arg_counter == 0) csi_codes[0] = 1;
                y = csi_codes[0] - 1;
                term_cursor_set(x, y);
                state = ST_NORMAL;
            }
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
                        term_put_char(xx, y, ' ');
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
            else if (c == 'L') {
                // IL: Insert Lines
                if (arg_counter == 0) csi_codes[0] = 1;
                term_shift_down(csi_codes[0]);
                state = ST_NORMAL;
            }
            else if (c == 'M') {
                // DL: Delete Lines
                if (arg_counter == 0) csi_codes[0] = 1;
                term_shift_up(csi_codes[0]);
                state = ST_NORMAL;
            }
            else if (c == 'n') {
                // DSR: Device Status Report
                if (csi_codes[0] == 5) {
                    serial_puts("\e[0n"); // Ready
                }
                else if (csi_codes[0] == 6) {
                    term_report_cursor(dec_set);
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
                if (arg_counter == 0) csi_codes[0] = 1;
                int shift = csi_codes[0];
                term_shift_right(shift);
                state = ST_NORMAL;
            }
            else if (c == 'P') {
                // DCH: Delete Character
                if (arg_counter == 0) csi_codes[0] = 1;
                int shift = csi_codes[0];
                if (shift > (TERM_WIDTH - x))
                    shift = TERM_WIDTH - x;
                for (int xx = x; xx < (TERM_WIDTH - shift); xx++) {
                    term_state_back->textmap[y][xx] = term_state_back->textmap[y][xx + shift];
                    term_state_back->colormap[y][xx] = term_state_back->colormap[y][xx + shift];
                    term_state_back->flagmap[y][xx] = term_state_back->flagmap[y][xx + shift];
                }
                for (int xx = TERM_WIDTH - shift; xx < TERM_WIDTH; xx++) {
                    term_state_back->textmap[y][xx] = ' ';
                    term_state_back->colormap[y][xx] = current_color;
                    term_state_back->flagmap[y][xx] = current_flag;
                }
                term_state_dirty = true;
                state = ST_NORMAL;
            }
            else if (c == 'X') {
                // ECH: Erase Character
                if (arg_counter == 1) {
                    int shift = csi_codes[0];
                    if (shift > (TERM_WIDTH - x))
                        shift = TERM_WIDTH - x;
                    for (int xx = x; xx < x + shift; xx++) {
                        term_state_back->textmap[y][xx] = ' ';
                        term_state_back->colormap[y][xx] = current_color;
                        term_state_back->flagmap[y][xx] = current_flag;
                    }
                    term_state_dirty = true;
                }
                state = ST_NORMAL;
            }
            else if (c == 'S') {
                // SU: Shift Up
                if (arg_counter == 0) csi_codes[0] = 1;
                int y = term_state_back->y;
                term_state_back->y = 0;
                term_shift_up(csi_codes[0]);
                term_state_back->y = y;
                state = ST_NORMAL;
            }
            else if (c == 'T') {
                // SD: Shift Down
                if (arg_counter == 0) csi_codes[0] = 1;
                int y = term_state_back->y;
                term_state_back->y = 0;
                term_shift_down(csi_codes[0]);
                term_state_back->y = y;
                state = ST_NORMAL;
            }
            else if (c == 'Z') {
                // CBT
                if (arg_counter == 0) csi_codes[0] = 1;
                for (int i = 0; i < csi_codes[0]; i++) {
                    term_backward_tab();
                }
                state = ST_NORMAL;
            }
            else if (c == 'b') {
                // REP: Repeat last graph char
                if (arg_counter == 0) csi_codes[0] = 1;
                state = ST_NORMAL;
                for (int i = 0; i < csi_codes[0]; i++) {
                    term_process_char(last_graph_char);
                }
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
                fprintf(stderr, "Unsupported CSI seq: %c (%d)", c, c);
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
    else if (state == ST_OSC_SEQ) {
        if ((c >= 0x30) && (c <= 0x39)) {
            // reuse CSI buffer
            csi[chr_counter++] = c;
            if (chr_counter > 4) {
                fprintf(stderr, "OSC sequence argument too long");
                state = ST_NORMAL;
            }
        }
        else if (c == ';') {
            // Start to receive the argument
            csi[chr_counter] = '\0';
            osc_type = atoi(csi);
            state = ST_OSC_PAR;
        }
        else {
            fprintf(stderr, "Unexpected char in OSC: %d", c);
            state = ST_NORMAL;
        }
    }
    else if (state == ST_OSC_PAR) {
        if (c == 0x07) {
            if (osc_type == 0) {
                // Set Icon and Window Title
                // Ignore
            }
            else if (osc_type == 1) {
                // Set Icon
                // Ignore
            }
            else if (osc_type == 2) {
                // Set Window Title
                // Ignore
            }
            else {
                fprintf(stderr, "Unsupported OSC seq: %d", osc_type);
            }
            state = ST_NORMAL;
        }
    }
}

void term_process_string(char *str) {
    while (*str) {
        term_process_char((uint8_t)(*str++));
    }
}

void term_full_reset(void) {
    term_reset();
    memset(&term_state_back_alternate, 0, sizeof(term_state_back_alternate));
}