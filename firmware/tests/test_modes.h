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

TEST_VECTOR test_mode_insert1 = {
    .name = "mode insert 1",
    .input_sequence = "Hello, world!\nSecond line\e[2D\e[A\e[4h123\e[4l",
    .expected_screen = {
        "Hello, wo123rld!",
        "Second line",
        0
    },  
    .expected_serial = "",
    .expected_cursor_x = 12,
    .expected_cursor_y = 0
};

TEST_VECTOR test_mode_insert2 = {
    .name = "mode insert 2",
    .input_sequence = "Hello, world!\nSecond line\e[2D\e[A\e[4h1234567891011289053489058349068368596845060523423290584395843085906854085\e[4l",
    .expected_screen = {
        "Hello, wo12345678910112890534890583490683685968450605234232905843958430859068540",
        "85Second line",
        0
    },  
    .expected_serial = "",
    .expected_cursor_x = 2,
    .expected_cursor_y = 1
};