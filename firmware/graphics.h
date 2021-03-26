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

#define FLAG_BOLD (0x80)
#define FLAG_ITALIC (0x40)
#define FLAG_UNDERLINE (0x20)
#define FLAG_STHROUGH (0x10)
#define FLAG_SLOWBLINK (0x08)
#define FLAG_FASTBLINK (0x04)
#define FLAG_INVERT (0x02)

void graph_put_pixel(int x, int y, int c);
void graph_fill_rect(int x1, int y1, int x2, int y2, int c);
void graph_put_char(int x, int y, char c, char cl_fg, char cl_bg, char flags);
void graph_put_char_small(int x, int y, char c, char cl_fg, char cl_bg);