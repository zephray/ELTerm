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
#pragma once

// Platform specific definition

// Additional line for scrolling
#define TERM_WIDTH 80
#define TERM_HEIGHT 30
#define TERM_BUF_HEIGHT (TERM_HEIGHT+2)

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

typedef struct {
    char textmap[TERM_BUF_HEIGHT][TERM_WIDTH];
    char flagmap[TERM_BUF_HEIGHT][TERM_WIDTH];
    char colormap[TERM_BUF_HEIGHT][TERM_WIDTH];
    int x, y, y_offset;
} TERM_STATE;

extern TERM_STATE *term_state_back; // State front is provided in the front end
extern bool term_state_dirty; // Set by termcore, clear by the front end

// Mode exposed to front end
extern bool mode_app_keypad;
extern bool mode_app_cursor;
extern bool mode_cursor_blinking;
extern bool mode_show_cursor;

// Needs to be implemented by user:
extern void serial_puts(char *string);

void term_full_reset(void);
void term_process_char(uint8_t c);
void term_process_string(char *str);
