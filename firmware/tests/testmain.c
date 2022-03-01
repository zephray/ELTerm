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
#include <stdlib.h>
#include <string.h> 
#include <stdint.h>
#include <stdbool.h>
#include <termios.h>
#include "../termcore.h"
#include "tests.h"

char *serial_out;
int serial_out_len = 0;

void serial_puts(char *string) {
    int len = strlen(string);
    serial_out = realloc(serial_out, serial_out_len + len + 1);
    memcpy(serial_out + serial_out_len, string, len + 1);
}

bool strcmp_with_null(char *expected, char *actual) {
    char c;
    if (!expected)
        return true;
    while (*expected) {
        if (!actual)
            return false;
        if (*actual == 0x00)
            *actual = 0x20;
        if (*expected != *actual)
            return false;
        expected++;
        actual++;
    }
    return true;
}

bool runtest(TEST_VECTOR *test) {
    printf("Testing %s...\n", test->name);
    term_full_reset();
    if (serial_out) {
        free(serial_out);
    }
    serial_out_len = 0;
    term_process_string(test->input_sequence);
    if (!strcmp_with_null(test->expected_serial, serial_out)) {
        printf("Serial output failed to match. Expected %s, got %s\n",
                test->expected_serial, serial_out);
        return false;
    }
    for (int i = 0; i < TERM_BUF_HEIGHT; i++) {
        if (!strcmp_with_null(test->expected_screen[i], term_state_back->textmap[i])) {
            printf("Screen output failed to match on line %d. Expected:\n%s\nGot:\n%s\n",
                    i, test->expected_screen[i], term_state_back->textmap[i]);
            return false;
        }
    }
    if (test->expected_cursor_x != term_state_back->x) {
        printf("Cursor X failed to match. Expected %d, got %d\n",
                test->expected_cursor_x, term_state_back->x);
        return false;
    }
    if (test->expected_cursor_y != term_state_back->y) {
        printf("Cursor Y failed to match. Expected %d, got %d\n",
                test->expected_cursor_y, term_state_back->y);
        return false;
    }
    return true;
}

void putline(char *str) {
    while (*str) {
        putchar(*str++);
    }
}

void runtestOnTerminal(TEST_VECTOR *test) {
    putline("\ec");
    putline(test->input_sequence);
    putline("\e[6n");
    char c;
    char buf[5];
    int i = 0;
    int x, y;
    struct termios t;
    tcgetattr(0, &t);
    t.c_lflag &= ~ECHO;
    tcsetattr(0, TCSANOW, &t);
    c = getchar();
    if (c != '\e')
        return;
    c = getchar();
    if (c != '[')
        return;
    do {
        c = getchar();
        if (c == ';') {
            buf[i] = '\0';
            y = atoi(buf);
            i = 0;
        }
        else if (c == 'R') {
            buf[i] = '\0';
            x = atoi(buf);
            break;
        }
        else {
            buf[i++] = c;
        }
    } while(1);
    printf("\nEnd X: %d, End Y: %d\n", x - 1, y - 1);
    t.c_lflag |= ECHO;
    tcsetattr(0, TCSANOW, &t);
}

int main(int argc, char *argv[]) {
#if 1
    int successCount = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        bool result = runtest(tests[i]);
        if (result) successCount++; 
    }
    printf("%d of %d tests passed.\n", successCount, TEST_COUNT);
#else
    runtestOnTerminal(tests[42]);
#endif

    return 0;
}