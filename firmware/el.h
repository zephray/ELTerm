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

#define VSYNC_PIN (2)
#define HSYNC_PIN (3)
#define PIXCLK_PIN (4)
// UD0-3: 5-8
#define UD0_PIN (5)
#define UD1_PIN (6)
#define UD2_PIN (7)
#define UD3_PIN (8)
// LD0-3: 9-12
#define LD0_PIN (9)
#define LD1_PIN (10)
#define LD2_PIN (11)
#define LD3_PIN (12)

// PIO related
#define EL_UDATA_SM (0)
#define EL_LDATA_SM (1)

// Screen related
#define EL_TARGET_PIXCLK (4500000)

#define SCR_WIDTH (640)
#define SCR_HEIGHT (480)
#define SCR_LINE_TRANSFERS (SCR_WIDTH / 4)
#define SCR_STRIDE (SCR_WIDTH / 8)
#define SCR_STRIDE_WORDS (SCR_WIDTH / 32)
#define SCR_REFRESH_LINES (SCR_HEIGHT / 2)

// Public variables and functions
extern unsigned char framebuf_bp0[SCR_STRIDE * SCR_HEIGHT];
extern unsigned char framebuf_bp1[SCR_STRIDE * SCR_HEIGHT];

void el_start();