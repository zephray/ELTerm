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

typedef struct {
    char *name;
    char *input_sequence;
    char *expected_screen[TERM_BUF_HEIGHT];
    char *expected_serial;
    int expected_cursor_x;
    int expected_cursor_y;
} TEST_VECTOR;

#include "test_basics.h"
#include "test_basic_escape.h"
#include "test_csi.h"
#include "test_modes.h"

TEST_VECTOR *tests[] = {
    &test_text_input,
    &test_backspace,
    &test_newline,
    &test_wrap1,
    &test_wrap2,
    &test_wrap3,
    &test_tab,
    &test_esc_ind,
    &test_esc_nel,
    &test_esc_ri,
    &test_esc_decscrc,
    &test_csi_ich1,
    &test_csi_ich2,
    &test_csi_cuu1,
    &test_csi_cuu2,
    &test_csi_cud1,
    &test_csi_cud2,
    &test_csi_cuf1,
    &test_csi_cuf2,
    &test_csi_cub1,
    &test_csi_cub2,
    &test_csi_cnl1,
    &test_csi_cpl1,
    &test_csi_cha,
    &test_csi_cup,
    &test_csi_cht,
    &test_csi_ed1,
    &test_csi_ed2,
    &test_csi_ed3,
    &test_csi_el1,
    &test_csi_el2,
    &test_csi_el3,
    &test_csi_il1,
    &test_csi_dl1,
    &test_csi_dch1,
    &test_csi_dch2,
    &test_csi_su,
    &test_csi_sd,
    &test_csi_ech,
    &test_csi_cbt,
    &test_csi_rep,
    &test_mode_insert1,
    &test_mode_insert2
};

#define TEST_COUNT (int)(sizeof(tests) / sizeof(TEST_VECTOR *))