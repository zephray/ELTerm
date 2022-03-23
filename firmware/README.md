# Introduction

This folder holds the firmware for the ELterm project. It's designed to run on the ELterm hardware. The code could be compiled with RP2040 SDK.

To build, do:

```
mkdir build
cd build
cmake ..
make
```

Some tests are provided in the tests folder. Running ```make && ./test``` to run all the tests.

Additionally, a PC emulator is provided in the pc folder. Running ```make``` to build the firmware for running on Linux/ macOS. The PC emulator is partially based on https://github.com/MurphyMc/lilt, which is also licensed under MIT.

# Features

## Supported
- BS: Ctrl-H
- CR: Ctrl-M
- FF: Ctrl-L
- LF: Ctrl-J
- SP: Space
- TAB: Ctrl-I
- VT: Ctrl-K
- DECSC: ESC 7, Save Cursor
- DECRC: ESC 8, Restore Cursor
- DECPAM: ESC =, Application Keypad
- DECPNM: ESC >, Normal Keypad
- RIS: ESC c, Full Reset
- ICH: CSI Ps @, Insert Ps (Blank) Character(s)
- CUU: CSI Ps A, Cursor Up Ps Times
- CUD: CSI Ps B, Cursor Down Ps Times
- CUF: CSI Ps C, Cursor Forward Ps Times
- CUB: CSI Ps D, Cursor Backward Ps Times
- CNL: CSI Ps E, Cursor Next Line Ps Times
- CPL: CSI Ps F, Cursor Preceding Line Ps Times
- CHA: CSI Ps G, Cursor Character Absolute \[column\]
- CUP: CSI Ps ; Ps H, Cursor Position \[row;column\]
- CHT: CSI Ps I, Cursor Forward Tabulation Ps tab stops
- ED: CSI Ps J, Erase in Display
- EL: CSI Ps K, Erase in Line
- IL: CSI Ps L, Insert Ps Line(s)
- DL: CSI Ps M, Delete Ps Line(s)
- DCH: CSI Ps P, Delete Ps Character(s)
- SU: CSI Ps S, Scroll up Ps lines
- SD: CSI Ps T, Scroll down Ps lines
- ECH: CSI Ps X, Erase Ps Character(s)
- CBT: CSI Ps Z, Cursor Backward Tabulation Ps tab stops
- HPA: CSI Pm `, Character Position Absolute \[column\]
- REP: CSI Ps b, Repeat the preceding graphic character Ps times
- Primary DA: CSI Ps c, Send Device Attributes
- VPA: CSI Pm d, Line Position Absolute \[row\]
- HVP: CSI Ps ; Ps f, Horizontal and Vertical Position \[row;column\]
- AM: CSI 2 h, Keyboard Action Mode
- IRM: CSI 4 h, Insert Mode
- SRM: CSI 12 h, Send/receive
- LNM: CSI 20 h, Automatic Newline
- DECCKM: CSI ? 1 h, Application Cursor Keys
- DECAWM: CSI ? 7 h, Wraparound Mode
- att610: CSI ? 12 h, Start Blinking Cursor
- DECTCEM: CSI ? 25 h, Show Cursor
- 47: CSI ? 47 h, Use Alternate Screen Buffer
- 1047: CSI ? 1047 h, Use Alternate Screen Buffer
- 1048: CSI ? 1048 h, Save cursor as in DECSC
- 1049: CSI ? 1049 h, Save cursor as in DECSC and use Alternate Screen Buffer, clearing it first
- SGR 0: CSI 0 m, Normal Display
- SGR 1: CSI 1/22 m, Bold Display
- SGR 4: CSI 4/24 m, Underlined Display
- SGR 5: CSI 5/25 m, Blink
- SGR 7: CSI 7/27 m, Inverse Display
- SGR 9: CSI 9/26 m, Strike-through Display
- SGR FG: CSI 30-37/39 m, Set foreground color
- SGR BG: CSI 40-47/49 m, Set background color
- SGR FG16: CSI 90-97 m, Set foreground color (16 color mode)
- SGR BG16: CSI 100-107 m, Set background color (16 color mode)
- DSR: CSI 5 n, Status Report
- CPR: CSI 6 n, Report Cursor Position
- DECSTR: CSI ! p, Soft Terminal Reset

## Not Supported
- ENQ: Ctrl-E
- SO: Ctrl-N
- SI: Ctrl-O
- S7C1T: ESC SP F
- S8C1T: ESC SP G
- dpANS X3.134.1: ESC SP L/M/N
- DECDHL: ESC # 3, DEC double-height line, top half
- DECDHL: ESC # 4, DEC double-height line, bottom half
- DECSWL: ESC # 5, DEC single-width line
- DECDWL: ESC # 6, DEC double-width line
- DECALN: ESC # 8, DEC Screen Alignment Test
- ISO2022: ESC % @, Select default character set, ISO 8859-1
- ISO2022: ESC % G, Select UTF-8 character set
- ISO2022: ESC ( C, Designate G0 Character Set
- ISO2022: ESC ) C, Designate G1 Character Set
- ISO2022: ESC * C, Designate G2 Character Set
- ISO2022: ESC + C, Designate G3 Character Set
- LS2: ESC n, Invoke the G2 Character Set as GL
- LS3: ESC o, Invoke the G3 Character Set as GL
- LS3R: ESC |, Invoke the G3 Character Set as GR
- LS2R: ESC }, Invoke the G2 Character Set as GR
- LS1R: ESC ~, Invoke the G1 Character Set as GR
- APC: Application Program-Control functions
- DECUDK: DCS Ps;Ps|Pt ST: User-Defined Keys
- DECRQSS: DCS $ q Pt ST: Request Status String
- DECSED: CSI ? Ps J, Erase in Display
- DECSEL: CSI ? Ps K, Erase in Line
- Mouse Tracking: CSI Ps ; Ps ; Ps ; Ps ; Ps T
- TBC: CSI Ps g, Tab Clear
- DECANM: CSI ? 2 h, Designate USASCII for character sets G0-G3
- DECANM: CSI ? 2 h, Designate USASCII for character sets G0-G3
- DECCOLM: CSI ? 3 h, 132 Column Mode
- DECSCLM: CSI ? 4 h, Smooth (Slow) Scroll
- DECSCNM: CSI ? 5 h, Reverse Video
- DECOM: CSI ? 6 h, Origin Mode
- DECARM: CSI ? 8 h, Auto-repeat Keys
- Mouse Tracking: CSI ? 9 h: Send Mouse X & Y on button press
- DECPFF: CSI ? 18 h, Print form feed
- DECPEX: CSI ? 19 h, Set print extent to full screen
- DECTEK: CSI ? 38 h, Enter Tektronix Mode
- DECNRCM: CSI ? 42 h, Enable Nation Replacement Character sets
- DECNKM: CSI ? 66 h, Application Keypad
- DECBKM: CSI ? 67 h, Backarrow key sends backspace
- Mouse Tracking: CSI ? 1000 h: Send Mouse X & Y on button press and release
- SGR 8: CSI 8/28 m, Invisible Display
- SGR FG256: CSI 38;5;Ps m, Set foreground color (256 color mode)
- SGR BG256: CSI 48;5;Ps m, Set background color (256 color mode)
- DECDSR: CSI ? Ps n, DEC-specific device status report
- DECSCL: CSI Ps ; Ps " p, Set conformance level
- DECSCA: CSI Ps " q, Select character protection attribute
- DECSTBM: CSI Ps ; Ps r, Set scrolling region
- DECCARA: CSI Pt ; Pl ; Pb ; Pr ; Ps $ r, Clear attributes in rectangular area
- DECCRA: CSI Pt ; Pl ; Pb ; Pr ; Pp ; Pt ; Pl ; Pp $ v, Copy rectangular area
- DECEFR: CSI Pt ; Pl ; Pb ; Pr ' w, Enable filter rectangle
- DECREQTPARM: CSI Ps x, Request terminal parameters
- DECFRA: CSI Pc ; Pt ; Pl ; Pb ; Pr $ x, Fill rectangular area
- DECELR: CSI Ps ; Pu ' z, Enable locator reporting
- DECERA: CSI Pt ; Pl ; Pb ; Pr $ z, Erase Rectangular Area

## Ignored
- BEL: Ctrl-G
- 2004: CSI ? 2004 h, Set bracketed paste mode
- OSC: Operating System Commands

# License

MIT