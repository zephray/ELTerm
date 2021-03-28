//
// Copyright 2021 Wenting Zhang <zephray@outlook.com>
// Copyright 2019 Ha Thach (tinyusb.org)
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "bsp/board.h"
#include "tusb.h"
#include "serial.h"
#include "terminal.h"
#include "usbhid.h"

//--------------------------------------------------------------------+
// USB HID
//--------------------------------------------------------------------+
#if CFG_TUH_HID_KEYBOARD

CFG_TUSB_MEM_SECTION static hid_keyboard_report_t usb_keyboard_report;

// look up new key in previous keys
static inline bool find_key_in_report(hid_keyboard_report_t const *p_report, uint8_t keycode) {
    for (uint8_t i = 0; i < 6; i++) {
        if (p_report->keycode[i] == keycode) return true;
    }

    return false;
}

static inline void process_kbd_report(hid_keyboard_report_t const *p_new_report) {
    static hid_keyboard_report_t prev_report = {0, 0, {0}}; // previous report to check key released

    //------------- example code ignore control (non-printable) key affects -------------//
    for (uint8_t i = 0; i < 6; i++) {
        if (p_new_report->keycode[i]) {
            if (find_key_in_report(&prev_report, p_new_report->keycode[i])) {
                // exist in previous report means the current key is holding
            } else {
                // not existed in previous report means the current key is pressed
                bool const is_shift =
                        p_new_report->modifier & (KEYBOARD_MODIFIER_LEFTSHIFT | KEYBOARD_MODIFIER_RIGHTSHIFT);
                bool const is_ctrl =
                        p_new_report->modifier & (KEYBOARD_MODIFIER_LEFTCTRL | KEYBOARD_MODIFIER_RIGHTCTRL);
                uint8_t keycode = p_new_report->keycode[i];
                term_key_pressed(keycode, is_shift, is_ctrl);
            }
        }
        if (prev_report.keycode[i]) {
            if (!find_key_in_report(p_new_report, prev_report.keycode[i])) {
                // key released
                term_key_released(prev_report.keycode[i]);
            }
        }
    }

    prev_report = *p_new_report;
}

void tuh_hid_keyboard_mounted_cb(uint8_t dev_addr) {
    // application set-up
    term_process_string("USB Keyboard connected.\r\n");

    tuh_hid_keyboard_get_report(dev_addr, &usb_keyboard_report);
}

void tuh_hid_keyboard_unmounted_cb(uint8_t dev_addr) {
    // application tear-down
    term_process_string("USB Keyboard disconnected.\r\n");
}

// invoked ISR context
void tuh_hid_keyboard_isr(uint8_t dev_addr, xfer_result_t event) {
    (void) dev_addr;
    (void) event;
}

#endif

void usbhid_init() {
    tusb_init();
}

void usbhid_polling() {
    tuh_task();

    uint8_t const addr = 1;

#if CFG_TUH_HID_KEYBOARD
    if (tuh_hid_keyboard_is_mounted(addr)) {
        if (!tuh_hid_keyboard_is_busy(addr)) {
            process_kbd_report(&usb_keyboard_report);
            tuh_hid_keyboard_get_report(addr, &usb_keyboard_report);
        }
    }
#endif

#if CFG_TUH_HID_MOUSE
    if (tuh_hid_mouse_is_mounted(addr)) {
        if (!tuh_hid_mouse_is_busy(addr)) {
            process_mouse_report(&usb_mouse_report);
            tuh_hid_mouse_get_report(addr, &usb_mouse_report);
        }
    }
#endif
}