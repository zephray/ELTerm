//
// Copyright 2021 Wenting Zhang <zephray@outlook.com>
// Copyright 2019 Murphy McCauley
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
#include "SDL.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <sys/ioctl.h>
#include "pico/stdlib.h"
#include "tusb.h"
#include "../el.h"
#include "../termcore.h"
#include "../graphics.h"
#include "../terminal.h"

// serial functions provided for pc port:
extern void serial_rx_push(uint8_t ch);

#define TARGET_FPS (120)

SDL_Surface * screen;

int master_fd = -1;
int slave_fd = -1;

// Updated by graphics.c, used by main.c
unsigned char framebuf_bp0[SCR_STRIDE * SCR_BUF_HEIGHT];
unsigned char framebuf_bp1[SCR_STRIDE * SCR_BUF_HEIGHT];

// Updated by pcmain.c, used by terminal.c
volatile bool frame_sync = false;

// Updated by terminal.c, used by main.c
volatile int frame_scroll_lines = 0;

void serial_putc(char c) {
    if (master_fd < 0)
        return;
    if (write(master_fd, &c, 1) != 1) {
        perror("Couldn't send data");
    }
}

void serial_puts(char *s) {
    // Unlike uart_puts, this does not do any CR/LF conversion
    if (master_fd < 0)
        return;
    size_t len = strlen(s);
    if (write(master_fd, s, len) != len) {
        perror("Couldn't send data");
    }
}

void init_slave(char * cmd)
{
    char **argv = calloc(2, sizeof(char *));
    argv[0] = cmd;
    argv[1] = NULL;

    setenv("TERM", "ansi", 1);
    setenv("LANG", "", 1);

    char buf[10];
    sprintf(buf, "%i", TERM_HEIGHT);
    setenv("LINES", buf, 1);
    sprintf(buf, "%i", TERM_WIDTH);
    setenv("COLS", buf, 1);

    close(master_fd);
    dup2(slave_fd, 0);
    dup2(slave_fd, 1);
    dup2(slave_fd, 2);

    setsid();

    ioctl(slave_fd, TIOCSCTTY, 1);

    struct winsize ws = {0};
    ws.ws_row = TERM_HEIGHT;
    ws.ws_col = TERM_WIDTH;
    ws.ws_xpixel = SCR_WIDTH;
    ws.ws_ypixel = SCR_HEIGHT;
    ioctl(slave_fd, TIOCSWINSZ, &ws);

    close(slave_fd);

    execvp(argv[0], &argv[0]);
    exit(1); // Shouldn't be reached!
}

static bool init_master()
{
    //TODO: Proper handling of /etc/shells and so forth
    char * nshell = getenv("SHELL");
    char * cmd;
    if (nshell && strlen(nshell))
        cmd = nshell;
    else
        cmd = "/bin/sh";

    master_fd = posix_openpt(O_RDWR);
    if (master_fd < 0) return false;
    if (grantpt(master_fd)) return false;
    if (unlockpt(master_fd)) return false;

    slave_fd = open(ptsname(master_fd), O_RDWR);
    if (slave_fd < 0) return false;

    if (fork() == 0) {
        init_slave(cmd);
        _exit(1); // Shouldn't be reached!
    }

    // Parent
    close(slave_fd);

    return true;
}

static void render_screen() {
    uint32_t *fb = screen->pixels;

    for (int i = 0; i < SCR_HEIGHT; i++) {
        int y = (frame_scroll_lines + i) % SCR_BUF_HEIGHT;
        uint8_t *bp0 = &framebuf_bp0[SCR_STRIDE * y];
        uint8_t *bp1 = &framebuf_bp1[SCR_STRIDE * y];
        for (int j = 0; j < SCR_STRIDE; j++) {
            uint8_t b0 = *bp0++;
            uint8_t b1 = *bp1++;
            for (int z = 0; z < 8; z++) {
                uint32_t color;
                color = (b0 & 0x1) + (b1 & 0x1);
                if (color == 0)
                    color = 0x000000fful;
                else if (color == 1)
                    color = 0x008096fful;
                else
                    color = 0x00d9fffful;
                *fb++ = color;
                b0 >>= 1;
                b1 >>= 1;
            }
        }
    }
}

static uint8_t get_hid_key(SDLKey keysym) {
    if ((keysym >= SDLK_a) && (keysym <= SDLK_z)) {
        return HID_KEY_A + (keysym - SDLK_a);
    }
    else if ((keysym >= SDLK_1) && (keysym <= SDLK_9)) {
        return HID_KEY_1 + (keysym - SDLK_1);
    }
    else if (keysym == SDLK_0) {
        return HID_KEY_0;
    }
    else if ((keysym >= SDLK_F1) && (keysym <= SDLK_F12)) {
        return HID_KEY_F1 + (keysym - SDLK_F1);
    }
    else if ((keysym >= SDLK_KP1) && (keysym <= SDLK_KP9)) {
        return HID_KEY_KEYPAD_1 + (keysym - SDLK_KP1);
    }
    else if (keysym == SDLK_KP0) {
        return HID_KEY_KEYPAD_0;
    }
    else if (keysym == SDLK_RETURN) {
        return HID_KEY_ENTER;
    }
    else if (keysym == SDLK_ESCAPE) {
        return HID_KEY_ESCAPE;
    }
    else if (keysym == SDLK_BACKSPACE) {
        return HID_KEY_BACKSPACE;
    }
    else if (keysym == SDLK_TAB) {
        return HID_KEY_TAB;
    }
    else if (keysym == SDLK_SPACE) {
        return HID_KEY_SPACE;
    }
    else if (keysym == SDLK_MINUS) {
        return HID_KEY_MINUS;
    }
    else if (keysym == SDLK_EQUALS) {
        return HID_KEY_EQUAL;
    }
    else if (keysym == SDLK_LEFTBRACKET) {
        return HID_KEY_BRACKET_LEFT;
    }
    else if (keysym == SDLK_RIGHTBRACKET) {
        return HID_KEY_BRACKET_RIGHT;
    }
    else if (keysym == SDLK_BACKSLASH) {
        return HID_KEY_BACKSLASH;
    }
    else if (keysym == SDLK_SEMICOLON) {
        return HID_KEY_SEMICOLON;
    }
    else if (keysym == SDLK_COMMA) {
        return HID_KEY_COMMA;
    }
    else if (keysym == SDLK_PERIOD) {
        return HID_KEY_PERIOD;
    }
    else if (keysym == SDLK_SLASH) {
        return HID_KEY_SLASH;
    }
    else if (keysym == SDLK_CAPSLOCK) {
        return HID_KEY_CAPS_LOCK;
    }
    else if (keysym == SDLK_PRINT) {
        return HID_KEY_PRINT_SCREEN;
    }
    else if (keysym == SDLK_SCROLLOCK) {
        return HID_KEY_SCROLL_LOCK;
    }
    else if (keysym == SDLK_BREAK) {
        return HID_KEY_PAUSE;
    }
    else if (keysym == SDLK_INSERT) {
        return HID_KEY_INSERT;
    }
    else if (keysym == SDLK_HOME) {
        return HID_KEY_HOME;
    }
    else if (keysym == SDLK_PAGEUP) {
        return HID_KEY_PAGE_UP;
    }
    else if (keysym == SDLK_DELETE) {
        return HID_KEY_DELETE;
    }
    else if (keysym == SDLK_END) {
        return HID_KEY_END;
    }
    else if (keysym == SDLK_PAGEDOWN) {
        return HID_KEY_PAGE_DOWN;
    }
    else if (keysym == SDLK_RIGHT) {
        return HID_KEY_ARROW_RIGHT;
    }
    else if (keysym == SDLK_LEFT) {
        return HID_KEY_ARROW_LEFT;
    }
    else if (keysym == SDLK_DOWN) {
        return HID_KEY_ARROW_DOWN;
    }
    else if (keysym == SDLK_UP) {
        return HID_KEY_ARROW_UP;
    }
    else if (keysym == SDLK_NUMLOCK) {
        return HID_KEY_NUM_LOCK;
    }
    else if (keysym == SDLK_KP_DIVIDE) {
        return HID_KEY_KEYPAD_DIVIDE;
    }
    else if (keysym == SDLK_KP_MULTIPLY) {
        return HID_KEY_KEYPAD_MULTIPLY;
    }
    else if (keysym == SDLK_KP_MINUS) {
        return HID_KEY_KEYPAD_SUBTRACT;
    }
    else if (keysym == SDLK_KP_PLUS) {
        return HID_KEY_KEYPAD_ADD;
    }
    else if (keysym == SDLK_KP_ENTER) {
        return HID_KEY_KEYPAD_ENTER;
    }
    else if (keysym == SDLK_KP_PERIOD) {
        return HID_KEY_KEYPAD_DECIMAL;
    }
    else if (keysym == SDLK_KP_EQUALS) {
        return HID_KEY_KEYPAD_EQUAL;
    }
    else if (keysym == SDLK_BACKQUOTE) {
        return HID_KEY_GRAVE;
    }
    else return HID_KEY_NONE;
}

int main(int argc, char * argv[])
{
    SDL_Init(SDL_INIT_VIDEO);

    // Disable key repeat
    SDL_EnableKeyRepeat(0, 0);
    SDL_EnableUNICODE(1);

    SDL_WM_SetCaption("ELterm PC emulator", NULL);

    screen = SDL_SetVideoMode(SCR_WIDTH, SCR_HEIGHT, 32, false);
    assert(screen);

    if (!init_master()) return -1;

    bool quitting = false;
    uint32_t last_frame = SDL_GetTicks();

    term_init();

    while (!quitting) {
        bool got_sdl_event = false;

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quitting = true;
                break;
            }
            else if (event.type == SDL_KEYDOWN) {
                bool is_shift = !!(event.key.keysym.mod & (KMOD_LSHIFT | KMOD_RSHIFT));
                bool is_ctrl = !!(event.key.keysym.mod & (KMOD_LCTRL | KMOD_RCTRL));
                uint8_t key = get_hid_key(event.key.keysym.sym);
                if (key != HID_KEY_NONE)
                    term_key_pressed(key, is_shift, is_ctrl);
            }
            else if (event.type == SDL_KEYUP) {
                uint8_t key = get_hid_key(event.key.keysym.sym);
                if (key != HID_KEY_NONE)
                    term_key_released(key);
            }
        }

        if (quitting) break;

        fd_set fd_in;
        FD_ZERO(&fd_in);
        FD_SET(master_fd, &fd_in);
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 1 * 1000;
        //printf("%i\n", delay);

        int rv = select(master_fd+1, &fd_in, NULL, NULL, &tv);
        if (rv == -1) {
            if (errno == EINTR) {
                quitting = true;
                break;
            }
            perror("select()");
        }
        else if (rv != 0) {
            static char buf[512];
            rv = read(master_fd, buf, sizeof(buf));
            if (rv > 0) {
                //tmt_write(vt, buf, rv);
                for (int i = 0; i < rv; i++) {
                    char c = buf[i];
                    #if 0
                    char d[10] = {0};
                    d[0] = c;
                    if (c == '\x1b') strcpy(d, "\\e");
                    else if (c == '\r') strcpy(d, "\\r");
                    else if (c == '\n') strcpy(d, "\\n");
                    else if (c == '\\') strcpy(d, "\\\\");
                    write(2, d, strlen(d));
                    #endif
                    serial_rx_push(c);
                }
            }
            else {
                // Child terminated?
                quitting = true;
            }
        }

        uint32_t now = SDL_GetTicks();

        fake_picolib_tick(now);

        term_loop();

        if (now - last_frame > (1000 / TARGET_FPS)) {
            render_screen();
            SDL_Flip(screen);
            frame_sync = true;
            last_frame = now;
        }
    }

    SDL_Quit();
    return 0;
}