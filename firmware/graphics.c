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
#include "pico/stdlib.h"
#include "el.h"
#include "font.h"

//#define GREYSCALE_3FRAME
#define GREYSCALE_2FRAME

static void _putpixel_bp(unsigned char *buf, int x, int y, int c) {
    if (c)
        buf[SCR_STRIDE * y + x / 8] |= 1 << (x % 8);
    else
        buf[SCR_STRIDE * y + x / 8] &= ~(1 << (x % 8));
}

void graph_put_pixel(int x, int y, int c) {
#ifdef GREYSCALE_3FRAME
    if (c == 3) {
        _putpixel_bp(framebuf_bp0, x, y, 1);
        _putpixel_bp(framebuf_bp1, x, y, 1);
        _putpixel_bp(framebuf_bp2, x, y, 1);
    }
    else if (c == 2) {
        if ((x & 1) ^ (y & 1)) {
            _putpixel_bp(framebuf_bp0, x, y, 1);
            _putpixel_bp(framebuf_bp1, x, y, 1);
            _putpixel_bp(framebuf_bp2, x, y, 0);
        }
        else {
            _putpixel_bp(framebuf_bp0, x, y, 0);
            _putpixel_bp(framebuf_bp1, x, y, 1);
            _putpixel_bp(framebuf_bp2, x, y, 1);
        }
    }
    else if (c == 1) {
        if ((x & 1) ^ (y & 1)) {
            _putpixel_bp(framebuf_bp0, x, y, 1);
            _putpixel_bp(framebuf_bp1, x, y, 0);
            _putpixel_bp(framebuf_bp2, x, y, 0);
        }
        else {
            _putpixel_bp(framebuf_bp0, x, y, 0);
            _putpixel_bp(framebuf_bp1, x, y, 1);
            _putpixel_bp(framebuf_bp2, x, y, 0);
        }
    }
    else {
        _putpixel_bp(framebuf_bp0, x, y, 0);
        _putpixel_bp(framebuf_bp1, x, y, 0);
        _putpixel_bp(framebuf_bp2, x, y, 0);
    }
#elif defined (GREYSCALE_2FRAME)
    if (c == 7) {
        _putpixel_bp(framebuf_bp0, x, y, 1);
        _putpixel_bp(framebuf_bp1, x, y, 1);
    }
    else if (c == 4) {
        if ((x & 1) ^ (y & 1)) {
            _putpixel_bp(framebuf_bp0, x, y, 1);
            _putpixel_bp(framebuf_bp1, x, y, 1);
        }
        else {
            _putpixel_bp(framebuf_bp0, x, y, 0);
            _putpixel_bp(framebuf_bp1, x, y, 1);
        }
    }
    else if (c == 3) {
        if ((x & 1) ^ (y & 1)) {
            _putpixel_bp(framebuf_bp0, x, y, 1);
            _putpixel_bp(framebuf_bp1, x, y, 1);
        }
        else {
            _putpixel_bp(framebuf_bp0, x, y, 0);
            _putpixel_bp(framebuf_bp1, x, y, 0);
        }
    }
    else if (c == 2) {
        if ((x & 1) ^ (y & 1)) {
            _putpixel_bp(framebuf_bp0, x, y, 1);
            _putpixel_bp(framebuf_bp1, x, y, 0);
        }
        else {
            _putpixel_bp(framebuf_bp0, x, y, 0);
            _putpixel_bp(framebuf_bp1, x, y, 1);
        }
    }
    else if (c == 1) {
        if ((x & 1) ^ (y & 1)) {
            _putpixel_bp(framebuf_bp0, x, y, 1);
            _putpixel_bp(framebuf_bp1, x, y, 0);
        }
    }
    else {
        _putpixel_bp(framebuf_bp0, x, y, 0);
        _putpixel_bp(framebuf_bp1, x, y, 0);
    }
#endif
}

void graph_fill_rect(int x1, int y1, int x2, int y2, int c) {
    for (int y = y1; y < y2; y++) {
        for (int x = x1; x < x2; x++) {
            graph_put_pixel(x, y, c);
        }
    }
}

void graph_put_mono(int x, int y, int width, int height, char *pimage, char cl_fg, char cl_bg)
{
	int i,j,k,pixel,rx=0,ry=0;
	unsigned char p;
	int iwidth = width/8 + (width % 8 ? 1:0);
	for (i = 0; i < height; ++i, pimage += iwidth)
	{
		ry = y + i;
		if (ry >= SCR_WIDTH)
            return;
		else if (ry < 0)
            continue;
		for (j = 0; j < iwidth; ++j) {
			p = pimage[j];
			for (k = 0; k < 8; ++k) {
				rx = x+(8-k)+8*j;
				pixel = p % 2;
				p>>=1;
				if (pixel)
                    graph_put_pixel(rx - 1, ry, cl_fg);
				else
                    graph_put_pixel(rx - 1, ry, cl_bg);
			}
		}
	}
}

void graph_put_char(int x, int y, char c, char cl_fg, char cl_bg) {
    graph_fill_rect(x, y, x + 8, y + 2, cl_bg);
    graph_put_mono(x, y + 2, 8, 12, charMap_ascii[c] , cl_fg , cl_bg);
    graph_fill_rect(x, y + 14, x + 8, y + 16, cl_bg);
}

void graph_put_char_small(int x, int y, char c, char cl_fg, char cl_bg) {
    int i, j, p;
	for(i = 0; i < 6; i++)
	{
		for(j = 8; j > 0; j--)
		{
			p = charMap_ascii_mini[(unsigned char)c][i] << j ;
			if (p & 0x80)
                graph_put_pixel(x + i, y + 8 - j, cl_fg);
            else
                graph_put_pixel(x + i, y + 8 - j, cl_bg);
		}
	}
}