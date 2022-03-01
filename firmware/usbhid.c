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
#if CFG_TUH_HID

#define MAX_REPORT  4

static struct
{
  uint8_t report_count;
  tuh_hid_report_info_t report_info[MAX_REPORT];
}hid_info[CFG_TUH_HID];

// look up new key in previous keys
static inline bool find_key_in_report(hid_keyboard_report_t const *p_report, uint8_t keycode) {
    for (uint8_t i = 0; i < 6; i++) {
        if (p_report->keycode[i] == keycode) return true;
    }

    return false;
}

static void process_kbd_report(hid_keyboard_report_t const *p_new_report) {
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

static void process_generic_report(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
    (void) dev_addr;

    uint8_t const rpt_count = hid_info[instance].report_count;
    tuh_hid_report_info_t* rpt_info_arr = hid_info[instance].report_info;
    tuh_hid_report_info_t* rpt_info = NULL;

    if ( rpt_count == 1 && rpt_info_arr[0].report_id == 0) {
        // Simple report without report ID as 1st byte
        rpt_info = &rpt_info_arr[0];
    }
    else {
        // Composite report, 1st byte is report ID, data starts from 2nd byte
        uint8_t const rpt_id = report[0];

        // Find report id in the arrray
        for(uint8_t i=0; i<rpt_count; i++) {
            if (rpt_id == rpt_info_arr[i].report_id ) {
                rpt_info = &rpt_info_arr[i];
                break;
            }
        }

        report++;
        len--;
    }

    if (!rpt_info) {
        printf("Couldn't find the report info for this report !\r\n");
        return;
    }

    // For complete list of Usage Page & Usage checkout src/class/hid/hid.h. For examples:
    // - Keyboard                     : Desktop, Keyboard
    // - Mouse                        : Desktop, Mouse
    // - Gamepad                      : Desktop, Gamepad
    // - Consumer Control (Media Key) : Consumer, Consumer Control
    // - System Control (Power key)   : Desktop, System Control
    // - Generic (vendor)             : 0xFFxx, xx
    if ( rpt_info->usage_page == HID_USAGE_PAGE_DESKTOP ) {
        switch (rpt_info->usage) {
        case HID_USAGE_DESKTOP_KEYBOARD:
            // Assume keyboard follow boot report layout
            process_kbd_report( (hid_keyboard_report_t const*) report );
        break;

        case HID_USAGE_DESKTOP_MOUSE:
            // Assume mouse follow boot report layout
            //process_mouse_report( (hid_mouse_report_t const*) report );
        break;

        default: break;
        }
    }
}


void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance,
        uint8_t const* desc_report, uint16_t desc_len) {
    // application set-up
    term_printf("HID device address = %d, instance = %d is mounted\r\n",
            dev_addr, instance);

    // Interface protocol (hid_interface_protocol_enum_t)
    const char* protocol_str[] = { "None", "Keyboard", "Mouse" };
    uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);

    term_printf("HID Interface Protocol = %s\r\n", protocol_str[itf_protocol]);

    // By default host stack will use activate boot protocol on supported
    // interface. Therefore for this simple example, we only need to parse
    // generic report descriptor (with built-in parser)
    if ( itf_protocol == HID_ITF_PROTOCOL_NONE ) {
        hid_info[instance].report_count = tuh_hid_parse_report_descriptor(
            hid_info[instance].report_info, MAX_REPORT, desc_report, desc_len);
        term_printf("HID has %u reports \r\n", hid_info[instance].report_count);
    }

    // request to receive report
    // tuh_hid_report_received_cb() will be invoked when report is available
    if ( !tuh_hid_receive_report(dev_addr, instance) ) {
        term_printf("Error: cannot request to receive report\r\n");
    }
}

// Invoked when device with hid interface is un-mounted
void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
    // application tear-down
    term_printf("HID device address = %d, instance = %d is unmounted\r\n",
            dev_addr, instance);
}

// Invoked when received report from device via interrupt endpoint
void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance,
        uint8_t const* report, uint16_t len) {
    uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);

    switch (itf_protocol) {
    case HID_ITF_PROTOCOL_KEYBOARD:
        process_kbd_report( (hid_keyboard_report_t const*) report );
        break;

    case HID_ITF_PROTOCOL_MOUSE:
        //process_mouse_report( (hid_mouse_report_t const*) report );
        break;

    default:
        // Generic report requires matching ReportID and contents with previous
        // parsed report info
        process_generic_report(dev_addr, instance, report, len);
        break;
    }

    // continue to request to receive report
    if (!tuh_hid_receive_report(dev_addr, instance)) {
        term_printf("Error: cannot request to receive report\r\n");
    }
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