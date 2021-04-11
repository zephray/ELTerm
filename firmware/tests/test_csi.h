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

TEST_VECTOR test_csi_ich1 = {
    .name = "csi ich 1",
    .input_sequence = "Hello, world!\b\b\b\b\b\e[@",
    .expected_screen = {
        "Hello, w orld!",
        0
    },  
    .expected_serial = "",
    .expected_cursor_x = 8,
    .expected_cursor_y = 0
};

TEST_VECTOR test_csi_ich2 = {
    .name = "csi ich 2",
    .input_sequence = "Hello, world!\b\b\b\b\b\e[70@Test",
    .expected_screen = {
        "Hello, wTest                                                                  or",
        0
    },  
    .expected_serial = "",
    .expected_cursor_x = 12,
    .expected_cursor_y = 0
};

TEST_VECTOR test_csi_cuu1 = {
    .name = "csi cuu 1",
    .input_sequence = "Hello, world!\nTest\e[A12",
    .expected_screen = {
        "Hell12 world!",
        "Test",
        0
    },  
    .expected_serial = "",
    .expected_cursor_x = 6,
    .expected_cursor_y = 0
};

TEST_VECTOR test_csi_cuu2 = {
    .name = "csi cuu 2",
    .input_sequence = "Hello, world!\nTest1\nTest2\e[2A12",
    .expected_screen = {
        "Hello12world!",
        "Test1",
        "Test2",
        0
    },  
    .expected_serial = "",
    .expected_cursor_x = 7,
    .expected_cursor_y = 0
};

TEST_VECTOR test_csi_cud1 = {
    .name = "csi cud 1",
    .input_sequence = "Hello, world!\e[BTest",
    .expected_screen = {
        "Hello, world!",
        "             Test",
        0
    },  
    .expected_serial = "",
    .expected_cursor_x = 17,
    .expected_cursor_y = 1
};

TEST_VECTOR test_csi_cud2 = {
    .name = "csi cud 2",
    .input_sequence = "Hello, world!\e[2BTest",
    .expected_screen = {
        "Hello, world!",
        "",
        "             Test",
        0
    },  
    .expected_serial = "",
    .expected_cursor_x = 17,
    .expected_cursor_y = 2
};

TEST_VECTOR test_csi_cuf1 = {
    .name = "csi cuf 1",
    .input_sequence = "Hello, world!\e[CTest",
    .expected_screen = {
        "Hello, world! Test",
        0
    },  
    .expected_serial = "",
    .expected_cursor_x = 18,
    .expected_cursor_y = 0
};

TEST_VECTOR test_csi_cuf2 = {
    .name = "csi cuf 2",
    .input_sequence = "Hello, world!\e[100CTest",
    .expected_screen = {
        "Hello, world!                                                                  T",
        "est",
        0
    },  
    .expected_serial = "",
    .expected_cursor_x = 3,
    .expected_cursor_y = 1
};

TEST_VECTOR test_csi_cub1 = {
    .name = "csi cub 1",
    .input_sequence = "Hello, world!\e[100C\e[Da",
    .expected_screen = {
        "Hello, world!                                                                 a",
        0
    },  
    .expected_serial = "",
    .expected_cursor_x = 79,
    .expected_cursor_y = 0
};

TEST_VECTOR test_csi_cub2 = {
    .name = "csi cub 2",
    .input_sequence = "Hello, world!\e[100Dh",
    .expected_screen = {
        "hello, world!",
        0
    },  
    .expected_serial = "",
    .expected_cursor_x = 1,
    .expected_cursor_y = 0
};

TEST_VECTOR test_csi_cnl1 = {
    .name = "csi cnl",
    .input_sequence = "Hello, world!\e[2ETest",
    .expected_screen = {
        "Hello, world!",
        "",
        "Test",
        0
    },  
    .expected_serial = "",
    .expected_cursor_x = 4,
    .expected_cursor_y = 2
};

TEST_VECTOR test_csi_cpl1 = {
    .name = "csi cpl",
    .input_sequence = "Hello, world!\e[2ETest\e[2F123",
    .expected_screen = {
        "123lo, world!",
        "",
        "Test",
        0
    },  
    .expected_serial = "",
    .expected_cursor_x = 3,
    .expected_cursor_y = 0
};

TEST_VECTOR test_csi_cha = {
    .name = "csi cha",
    .input_sequence = "Hello, world!\e[20G123",
    .expected_screen = {
        "Hello, world!      123",
        0
    },  
    .expected_serial = "",
    .expected_cursor_x = 22,
    .expected_cursor_y = 0
};

TEST_VECTOR test_csi_cup = {
    .name = "csi cup",
    .input_sequence = "Hello, world!\e[2;20H123",
    .expected_screen = {
        "Hello, world!",
        "                   123",
        0
    },  
    .expected_serial = "",
    .expected_cursor_x = 22,
    .expected_cursor_y = 1
};

TEST_VECTOR test_csi_cht = {
    .name = "csi cht",
    .input_sequence = "Hello, world!\e[2I123\e[I123456\e[2I1",
    .expected_screen = {
        "Hello, world!           123     123456          1",
        0
    },  
    .expected_serial = "",
    .expected_cursor_x = 49,
    .expected_cursor_y = 0
};

TEST_VECTOR test_csi_ed1 = {
    .name = "csi ed 1",
    .input_sequence = "Hello, world!\nLine1\nLine2\nLine3\e[2A\e[2D\e[0J",
    .expected_screen = {
        "Hello, world!",
        "Lin  ",
        "     ",
        "     ",
        0
    },  
    .expected_serial = "",
    .expected_cursor_x = 3,
    .expected_cursor_y = 1
};

TEST_VECTOR test_csi_ed2 = {
    .name = "csi ed 2",
    .input_sequence = "Hello, world!\nLine1\nLine2\nLine3\e[2A\e[2D\e[1J",
    .expected_screen = {
        "             ",
        "    1",
        "Line2",
        "Line3",
        0
    },  
    .expected_serial = "",
    .expected_cursor_x = 3,
    .expected_cursor_y = 1
};

TEST_VECTOR test_csi_ed3 = {
    .name = "csi ed 3",
    .input_sequence = "Hello, world!\nLine1\nLine2\nLine3\e[2A\e[2D\e[2J",
    .expected_screen = {
        "             ",
        "     ",
        "     ",
        "     ",
        0
    },  
    .expected_serial = "",
    .expected_cursor_x = 3,
    .expected_cursor_y = 1
};

TEST_VECTOR test_csi_el1 = {
    .name = "csi el 1",
    .input_sequence = "Hello, world!\nLine1\nLine2\nLine3\e[2A\e[2D\e[0K",
    .expected_screen = {
        "Hello, world!",
        "Lin  ",
        "Line2",
        "Line3",
        0
    },  
    .expected_serial = "",
    .expected_cursor_x = 3,
    .expected_cursor_y = 1
};

TEST_VECTOR test_csi_el2 = {
    .name = "csi el 2",
    .input_sequence = "Hello, world!\nLine1\nLine2\nLine3\e[2A\e[2D\e[1K",
    .expected_screen = {
        "Hello, world!",
        "    1",
        "Line2",
        "Line3",
        0
    },  
    .expected_serial = "",
    .expected_cursor_x = 3,
    .expected_cursor_y = 1
};

TEST_VECTOR test_csi_el3 = {
    .name = "csi el 3",
    .input_sequence = "Hello, world!\nLine1\nLine2\nLine3\e[2A\e[2D\e[2K",
    .expected_screen = {
        "Hello, world!",
        "     ",
        "Line2",
        "Line3",
        0
    },  
    .expected_serial = "",
    .expected_cursor_x = 3,
    .expected_cursor_y = 1
};

TEST_VECTOR test_csi_il1 = {
    .name = "csi il",
    .input_sequence = "Hello, world!\nLine1\nLine2\nLine3\e[2A\e[2D\e[2L",
    .expected_screen = {
        "Hello, world!",
        "     ",
        "     ",
        "Line1",
        "Line2",
        "Line3",
        0
    },  
    .expected_serial = "",
    .expected_cursor_x = 3,
    .expected_cursor_y = 1
};

TEST_VECTOR test_csi_dl1 = {
    .name = "csi dl",
    .input_sequence = "Hello, world!\nLine1\nLine2\nLine3\e[2A\e[2D\e[2M",
    .expected_screen = {
        "Hello, world!",
        "Line3",
        "     ",
        "     ",
        "     ",
        0
    },  
    .expected_serial = "",
    .expected_cursor_x = 3,
    .expected_cursor_y = 1
};

TEST_VECTOR test_csi_dch1 = {
    .name = "csi dch 1",
    .input_sequence = "Hello, world!\nLine1\nLine2\nLine3\e[2A\e[2D\e[P",
    .expected_screen = {
        "Hello, world!",
        "Lin1 ",
        "Line2",
        "Line3",
        0
    },  
    .expected_serial = "",
    .expected_cursor_x = 3,
    .expected_cursor_y = 1
};

TEST_VECTOR test_csi_dch2 = {
    .name = "csi dch 2",
    .input_sequence = "Hello, world!\nLine1\nLine2\nLine3\e[2A\e[2D\e[80P",
    .expected_screen = {
        "Hello, world!",
        "Lin  ",
        "Line2",
        "Line3",
        0
    },  
    .expected_serial = "",
    .expected_cursor_x = 3,
    .expected_cursor_y = 1
};

TEST_VECTOR test_csi_su = {
    .name = "csi su",
    .input_sequence = "Hello, world!\nLine1\nLine2\nLine3\e[2A\e[2D\e[S",
    .expected_screen = {
        "Line1",
        "Line2",
        "Line3",
        0
    },  
    .expected_serial = "",
    .expected_cursor_x = 3,
    .expected_cursor_y = 1
};

TEST_VECTOR test_csi_sd = {
    .name = "csi sd",
    .input_sequence = "Hello, world!\nLine1\nLine2\nLine3\e[2A\e[2D\e[2T",
    .expected_screen = {
        "             ",
        "             ",
        "Hello, world!",
        "Line1",
        "Line2",
        "Line3",
        0
    },  
    .expected_serial = "",
    .expected_cursor_x = 3,
    .expected_cursor_y = 1
};

TEST_VECTOR test_csi_ech = {
    .name = "csi ech",
    .input_sequence = "Hello, world!\nLine1\nLine2\nLine3\e[2A\e[3D\e[2X",
    .expected_screen = {
        "Hello, world!",
        "Li  1",
        "Line2",
        "Line3",
        0
    },  
    .expected_serial = "",
    .expected_cursor_x = 2,
    .expected_cursor_y = 1
};

TEST_VECTOR test_csi_cbt = {
    .name = "csi cbt",
    .input_sequence = "Hello, world! This is a long line.\e[2Z123",
    .expected_screen = {
        "Hello, world! This is a 123g line.",
        0
    },  
    .expected_serial = "",
    .expected_cursor_x = 27,
    .expected_cursor_y = 0
};

TEST_VECTOR test_csi_rep = {
    .name = "csi rep",
    .input_sequence = "Hello, world!\nLine1\nLine2\nLine3\e[2A\e[2D\e[4b",
    .expected_screen = {
        "Hello, world!",
        "Lin3333",
        "Line2",
        "Line3",
        0
    },  
    .expected_serial = "",
    .expected_cursor_x = 7,
    .expected_cursor_y = 1
};
