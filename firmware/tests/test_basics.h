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

TEST_VECTOR test_text_input = {
    .name = "text input",
    .input_sequence = "Hello, world!",
    .expected_screen = {
        "Hello, world!",
        0
    },  
    .expected_serial = "",
    .expected_cursor_x = 13,
    .expected_cursor_y = 0
};

TEST_VECTOR test_backspace = {
    .name = "backspace",
    .input_sequence = "Hello, world!\b.\b\b",
    .expected_screen = {
        "Hello, world.",
        0
    },  
    .expected_serial = "",
    .expected_cursor_x = 11,
    .expected_cursor_y = 0
};

TEST_VECTOR test_newline = {
    .name = "newline",
    .input_sequence = "Hello, world!\nLF\nLF\rCR\rCR\n\rTest\rCR",
    .expected_screen = {
        "Hello, world!",
        "LF",
        "CR",
        "CRst",
        0
    },  
    .expected_serial = "",
    .expected_cursor_x = 2,
    .expected_cursor_y = 3
};

TEST_VECTOR test_wrap1 = {
    .name = "wrap1",
    .input_sequence = "888888888888888888888888888888888888888888888888888888888888888888888888888888888",
    .expected_screen = {
        "88888888888888888888888888888888888888888888888888888888888888888888888888888888",
        "8",
        0
    },  
    .expected_serial = "",
    .expected_cursor_x = 1,
    .expected_cursor_y = 1
};

TEST_VECTOR test_wrap2 = {
    .name = "wrap2",
    .input_sequence = "88888888888888888888888888888888888888888888888888888888888888888888888888888888",
    .expected_screen = {
        "88888888888888888888888888888888888888888888888888888888888888888888888888888888",
        0
    },  
    .expected_serial = "",
    .expected_cursor_x = 79,
    .expected_cursor_y = 0
};

TEST_VECTOR test_wrap3 = {
    .name = "wrap3",
    .input_sequence = "88888888888888888888888888888888888888888888888888888888888888888888888888888888\b\b7",
    .expected_screen = {
        "88888888888888888888888888888888888888888888888888888888888888888888888888888788",
        0
    },  
    .expected_serial = "",
    .expected_cursor_x = 78,
    .expected_cursor_y = 0
};

TEST_VECTOR test_tab = {
    .name = "tab",
    .input_sequence = "123\t456\t78910111213\t141516\t17",
    .expected_screen = {
        "123     456     78910111213     141516  17",
        0
    },  
    .expected_serial = "",
    .expected_cursor_x = 42,
    .expected_cursor_y = 0
};
