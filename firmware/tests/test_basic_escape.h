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

TEST_VECTOR test_esc_ind = {
    .name = "esc ind",
    .input_sequence = "Hello, world!\eDTest",
    .expected_screen = {
        "Hello, world!",
        "             Test",
        0
    },  
    .expected_serial = "",
    .expected_cursor_x = 17,
    .expected_cursor_y = 1
};

TEST_VECTOR test_esc_nel = {
    .name = "esc nel",
    .input_sequence = "Hello, world!\eETest",
    .expected_screen = {
        "Hello, world!",
        "Test",
        0
    },  
    .expected_serial = "",
    .expected_cursor_x = 4,
    .expected_cursor_y = 1
};

TEST_VECTOR test_esc_ri = {
    .name = "esc ri",
    .input_sequence = "Hello, world!\nTest\eM12",
    .expected_screen = {
        "Hell12 world!",
        "Test",
        0
    },  
    .expected_serial = "",
    .expected_cursor_x = 6,
    .expected_cursor_y = 0
};
