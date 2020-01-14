-- hid event listing
local hid_events = {}

-- Event types

hid_events.types = {}

hid_events.types.EV_SYN = 0x00
hid_events.types.EV_KEY = 0x01
hid_events.types.EV_REL = 0x02
hid_events.types.EV_ABS = 0x03
hid_events.types.EV_MSC = 0x04
hid_events.types.EV_SW = 0x05
hid_events.types.EV_LED = 0x11
hid_events.types.EV_SND = 0x12
hid_events.types.EV_REP = 0x14
hid_events.types.EV_FF = 0x15
hid_events.types.EV_PWR = 0x16
hid_events.types.EV_FF_STATUS = 0x17

-- Input event codes

hid_events.codes = {}

-- Synchronization events.

hid_events.codes.SYN_REPORT = 0
hid_events.codes.SYN_CONFIG = 1
hid_events.codes.SYN_MT_REPORT = 2
hid_events.codes.SYN_DROPPED = 3

-- Keys and buttons

hid_events.codes.KEY_RESERVED = 0
hid_events.codes.KEY_ESC = 1
hid_events.codes.KEY_1 = 2
hid_events.codes.KEY_2 = 3
hid_events.codes.KEY_3 = 4
hid_events.codes.KEY_4 = 5
hid_events.codes.KEY_5 = 6
hid_events.codes.KEY_6 = 7
hid_events.codes.KEY_7 = 8
hid_events.codes.KEY_8 = 9
hid_events.codes.KEY_9 = 10
hid_events.codes.KEY_0 = 11
hid_events.codes.KEY_MINUS = 12
hid_events.codes.KEY_EQUAL = 13
hid_events.codes.KEY_BACKSPACE = 14
hid_events.codes.KEY_TAB = 15
hid_events.codes.KEY_Q = 16
hid_events.codes.KEY_W = 17
hid_events.codes.KEY_E = 18
hid_events.codes.KEY_R = 19
hid_events.codes.KEY_T = 20
hid_events.codes.KEY_Y = 21
hid_events.codes.KEY_U = 22
hid_events.codes.KEY_I = 23
hid_events.codes.KEY_O = 24
hid_events.codes.KEY_P = 25
hid_events.codes.KEY_LEFTBRACE = 26
hid_events.codes.KEY_RIGHTBRACE = 27
hid_events.codes.KEY_ENTER = 28
hid_events.codes.KEY_LEFTCTRL = 29
hid_events.codes.KEY_A = 30
hid_events.codes.KEY_S = 31
hid_events.codes.KEY_D = 32
hid_events.codes.KEY_F = 33
hid_events.codes.KEY_G = 34
hid_events.codes.KEY_H = 35
hid_events.codes.KEY_J = 36
hid_events.codes.KEY_K = 37
hid_events.codes.KEY_L = 38
hid_events.codes.KEY_SEMICOLON = 39
hid_events.codes.KEY_APOSTROPHE = 40
hid_events.codes.KEY_GRAVE = 41
hid_events.codes.KEY_LEFTSHIFT = 42
hid_events.codes.KEY_BACKSLASH = 43
hid_events.codes.KEY_Z = 44
hid_events.codes.KEY_X = 45
hid_events.codes.KEY_C = 46
hid_events.codes.KEY_V = 47
hid_events.codes.KEY_B = 48
hid_events.codes.KEY_N = 49
hid_events.codes.KEY_M = 50
hid_events.codes.KEY_COMMA = 51
hid_events.codes.KEY_DOT = 52
hid_events.codes.KEY_SLASH = 53
hid_events.codes.KEY_RIGHTSHIFT = 54
hid_events.codes.KEY_KPASTERISK = 55
hid_events.codes.KEY_LEFTALT = 56
hid_events.codes.KEY_SPACE = 57
hid_events.codes.KEY_CAPSLOCK = 58
hid_events.codes.KEY_F1 = 59
hid_events.codes.KEY_F2 = 60
hid_events.codes.KEY_F3 = 61
hid_events.codes.KEY_F4 = 62
hid_events.codes.KEY_F5 = 63
hid_events.codes.KEY_F6 = 64
hid_events.codes.KEY_F7 = 65
hid_events.codes.KEY_F8 = 66
hid_events.codes.KEY_F9 = 67
hid_events.codes.KEY_F10 = 68
hid_events.codes.KEY_NUMLOCK = 69
hid_events.codes.KEY_SCROLLLOCK = 70
hid_events.codes.KEY_KP7 = 71
hid_events.codes.KEY_KP8 = 72
hid_events.codes.KEY_KP9 = 73
hid_events.codes.KEY_KPMINUS = 74
hid_events.codes.KEY_KP4 = 75
hid_events.codes.KEY_KP5 = 76
hid_events.codes.KEY_KP6 = 77
hid_events.codes.KEY_KPPLUS = 78
hid_events.codes.KEY_KP1 = 79
hid_events.codes.KEY_KP2 = 80
hid_events.codes.KEY_KP3 = 81
hid_events.codes.KEY_KP0 = 82
hid_events.codes.KEY_KPDOT = 83

hid_events.codes.KEY_ZENKAKUHANKAKU = 85
hid_events.codes.KEY_102ND = 86
hid_events.codes.KEY_F11 = 87
hid_events.codes.KEY_F12 = 88
hid_events.codes.KEY_RO = 89
hid_events.codes.KEY_KATAKANA = 90
hid_events.codes.KEY_HIRAGANA = 91
hid_events.codes.KEY_HENKAN = 92
hid_events.codes.KEY_KATAKANAHIRAGANA = 93
hid_events.codes.KEY_MUHENKAN = 94
hid_events.codes.KEY_KPJPCOMMA = 95
hid_events.codes.KEY_KPENTER = 96
hid_events.codes.KEY_RIGHTCTRL = 97
hid_events.codes.KEY_KPSLASH = 98
hid_events.codes.KEY_SYSRQ = 99
hid_events.codes.KEY_RIGHTALT = 100
hid_events.codes.KEY_LINEFEED = 101
hid_events.codes.KEY_HOME = 102
hid_events.codes.KEY_UP = 103
hid_events.codes.KEY_PAGEUP = 104
hid_events.codes.KEY_LEFT = 105
hid_events.codes.KEY_RIGHT = 106
hid_events.codes.KEY_END = 107
hid_events.codes.KEY_DOWN = 108
hid_events.codes.KEY_PAGEDOWN = 109
hid_events.codes.KEY_INSERT = 110
hid_events.codes.KEY_DELETE = 111
hid_events.codes.KEY_MACRO = 112
hid_events.codes.KEY_MUTE = 113
hid_events.codes.KEY_VOLUMEDOWN = 114
hid_events.codes.KEY_VOLUMEUP = 115
hid_events.codes.KEY_POWER = 116
hid_events.codes.KEY_KPEQUAL = 117
hid_events.codes.KEY_KPPLUSMINUS = 118
hid_events.codes.KEY_PAUSE = 119
hid_events.codes.KEY_SCALE = 120

hid_events.codes.KEY_KPCOMMA = 121
hid_events.codes.KEY_HANGEUL = 122
hid_events.codes.KEY_HANGUEL = hid_events.codes.KEY_HANGEUL
hid_events.codes.KEY_HANJA = 123
hid_events.codes.KEY_YEN = 124
hid_events.codes.KEY_LEFTMETA = 125
hid_events.codes.KEY_RIGHTMETA = 126
hid_events.codes.KEY_COMPOSE = 127

hid_events.codes.KEY_STOP = 128
hid_events.codes.KEY_AGAIN = 129
hid_events.codes.KEY_PROPS = 130
hid_events.codes.KEY_UNDO = 131
hid_events.codes.KEY_FRONT = 132
hid_events.codes.KEY_COPY = 133
hid_events.codes.KEY_OPEN = 134
hid_events.codes.KEY_PASTE = 135
hid_events.codes.KEY_FIND = 136
hid_events.codes.KEY_CUT = 137
hid_events.codes.KEY_HELP = 138
hid_events.codes.KEY_MENU = 139
hid_events.codes.KEY_CALC = 140
hid_events.codes.KEY_SETUP = 141
hid_events.codes.KEY_SLEEP = 142
hid_events.codes.KEY_WAKEUP = 143
hid_events.codes.KEY_FILE = 144
hid_events.codes.KEY_SENDFILE = 145
hid_events.codes.KEY_DELETEFILE = 146
hid_events.codes.KEY_XFER = 147
hid_events.codes.KEY_PROG1 = 148
hid_events.codes.KEY_PROG2 = 149
hid_events.codes.KEY_WWW = 150
hid_events.codes.KEY_MSDOS = 151
hid_events.codes.KEY_COFFEE = 152
hid_events.codes.KEY_SCREENLOCK = hid_events.codes.KEY_COFFEE
hid_events.codes.KEY_ROTATE_DISPLAY = 153
hid_events.codes.KEY_DIRECTION = hid_events.codes.KEY_ROTATE_DISPLAY
hid_events.codes.KEY_CYCLEWINDOWS = 154
hid_events.codes.KEY_MAIL = 155
hid_events.codes.KEY_BOOKMARKS = 156
hid_events.codes.KEY_COMPUTER = 157
hid_events.codes.KEY_BACK = 158
hid_events.codes.KEY_FORWARD = 159
hid_events.codes.KEY_CLOSECD = 160
hid_events.codes.KEY_EJECTCD = 161
hid_events.codes.KEY_EJECTCLOSECD = 162
hid_events.codes.KEY_NEXTSONG = 163
hid_events.codes.KEY_PLAYPAUSE = 164
hid_events.codes.KEY_PREVIOUSSONG = 165
hid_events.codes.KEY_STOPCD = 166
hid_events.codes.KEY_RECORD = 167
hid_events.codes.KEY_REWIND = 168
hid_events.codes.KEY_PHONE = 169
hid_events.codes.KEY_ISO = 170
hid_events.codes.KEY_CONFIG = 171
hid_events.codes.KEY_HOMEPAGE = 172
hid_events.codes.KEY_REFRESH = 173
hid_events.codes.KEY_EXIT = 174
hid_events.codes.KEY_MOVE = 175
hid_events.codes.KEY_EDIT = 176
hid_events.codes.KEY_SCROLLUP = 177
hid_events.codes.KEY_SCROLLDOWN = 178
hid_events.codes.KEY_KPLEFTPAREN = 179
hid_events.codes.KEY_KPRIGHTPAREN = 180
hid_events.codes.KEY_NEW = 181
hid_events.codes.KEY_REDO = 182

hid_events.codes.KEY_F13 = 183
hid_events.codes.KEY_F14 = 184
hid_events.codes.KEY_F15 = 185
hid_events.codes.KEY_F16 = 186
hid_events.codes.KEY_F17 = 187
hid_events.codes.KEY_F18 = 188
hid_events.codes.KEY_F19 = 189
hid_events.codes.KEY_F20 = 190
hid_events.codes.KEY_F21 = 191
hid_events.codes.KEY_F22 = 192
hid_events.codes.KEY_F23 = 193
hid_events.codes.KEY_F24 = 194

hid_events.codes.KEY_PLAYCD = 200
hid_events.codes.KEY_PAUSECD = 201
hid_events.codes.KEY_PROG3 = 202
hid_events.codes.KEY_PROG4 = 203
hid_events.codes.KEY_DASHBOARD = 204
hid_events.codes.KEY_SUSPEND = 205
hid_events.codes.KEY_CLOSE = 206
hid_events.codes.KEY_PLAY = 207
hid_events.codes.KEY_FASTFORWARD = 208
hid_events.codes.KEY_BASSBOOST = 209
hid_events.codes.KEY_PRINT = 210
hid_events.codes.KEY_HP = 211
hid_events.codes.KEY_CAMERA = 212
hid_events.codes.KEY_SOUND = 213
hid_events.codes.KEY_QUESTION = 214
hid_events.codes.KEY_EMAIL = 215
hid_events.codes.KEY_CHAT = 216
hid_events.codes.KEY_SEARCH = 217
hid_events.codes.KEY_CONNECT = 218
hid_events.codes.KEY_FINANCE = 219
hid_events.codes.KEY_SPORT = 220
hid_events.codes.KEY_SHOP = 221
hid_events.codes.KEY_ALTERASE = 222
hid_events.codes.KEY_CANCEL = 223
hid_events.codes.KEY_BRIGHTNESSDOWN = 224
hid_events.codes.KEY_BRIGHTNESSUP = 225
hid_events.codes.KEY_MEDIA = 226

hid_events.codes.KEY_SWITCHVIDEOMODE = 227
hid_events.codes.KEY_KBDILLUMTOGGLE = 228
hid_events.codes.KEY_KBDILLUMDOWN = 229
hid_events.codes.KEY_KBDILLUMUP = 230

hid_events.codes.KEY_SEND = 231
hid_events.codes.KEY_REPLY = 232
hid_events.codes.KEY_FORWARDMAIL = 233
hid_events.codes.KEY_SAVE = 234
hid_events.codes.KEY_DOCUMENTS = 235

hid_events.codes.KEY_BATTERY = 236

hid_events.codes.KEY_BLUETOOTH = 237
hid_events.codes.KEY_WLAN = 238
hid_events.codes.KEY_UWB = 239

hid_events.codes.KEY_UNKNOWN = 240

hid_events.codes.KEY_VIDEO_NEXT = 241
hid_events.codes.KEY_VIDEO_PREV = 242
hid_events.codes.KEY_BRIGHTNESS_CYCLE = 243
hid_events.codes.KEY_BRIGHTNESS_AUTO = 244
hid_events.codes.KEY_BRIGHTNESS_ZERO = hid_events.codes.KEY_BRIGHTNESS_AUTO
hid_events.codes.KEY_DISPLAY_OFF = 245

hid_events.codes.KEY_WWAN = 246
hid_events.codes.KEY_WIMAX = hid_events.codes.KEY_WWAN
hid_events.codes.KEY_RFKILL = 247

hid_events.codes.KEY_MICMUTE = 248

hid_events.codes.BTN_MISC = 0x100
hid_events.codes.BTN_0 = 0x100
hid_events.codes.BTN_1 = 0x101
hid_events.codes.BTN_2 = 0x102
hid_events.codes.BTN_3 = 0x103
hid_events.codes.BTN_4 = 0x104
hid_events.codes.BTN_5 = 0x105
hid_events.codes.BTN_6 = 0x106
hid_events.codes.BTN_7 = 0x107
hid_events.codes.BTN_8 = 0x108
hid_events.codes.BTN_9 = 0x109

hid_events.codes.BTN_MOUSE = 0x110
hid_events.codes.BTN_LEFT = 0x110
hid_events.codes.BTN_RIGHT = 0x111
hid_events.codes.BTN_MIDDLE = 0x112
hid_events.codes.BTN_SIDE = 0x113
hid_events.codes.BTN_EXTRA = 0x114
hid_events.codes.BTN_FORWARD = 0x115
hid_events.codes.BTN_BACK = 0x116
hid_events.codes.BTN_TASK = 0x117

hid_events.codes.BTN_JOYSTICK = 0x120
hid_events.codes.BTN_TRIGGER = 0x120
hid_events.codes.BTN_THUMB = 0x121
hid_events.codes.BTN_THUMB2 = 0x122
hid_events.codes.BTN_TOP = 0x123
hid_events.codes.BTN_TOP2 = 0x124
hid_events.codes.BTN_PINKIE = 0x125
hid_events.codes.BTN_BASE = 0x126
hid_events.codes.BTN_BASE2 = 0x127
hid_events.codes.BTN_BASE3 = 0x128
hid_events.codes.BTN_BASE4 = 0x129
hid_events.codes.BTN_BASE5 = 0x12a
hid_events.codes.BTN_BASE6 = 0x12b
hid_events.codes.BTN_DEAD = 0x12f

hid_events.codes.BTN_GAMEPAD = 0x130
hid_events.codes.BTN_SOUTH = 0x130
hid_events.codes.BTN_A = hid_events.codes.BTN_SOUTH
hid_events.codes.BTN_EAST = 0x131
hid_events.codes.BTN_B = hid_events.codes.BTN_EAST
hid_events.codes.BTN_C = 0x132
hid_events.codes.BTN_NORTH = 0x133
hid_events.codes.BTN_X = hid_events.codes.BTN_NORTH
hid_events.codes.BTN_WEST = 0x134
hid_events.codes.BTN_Y = hid_events.codes.BTN_WEST
hid_events.codes.BTN_Z = 0x135
hid_events.codes.BTN_TL = 0x136
hid_events.codes.BTN_TR = 0x137
hid_events.codes.BTN_TL2 = 0x138
hid_events.codes.BTN_TR2 = 0x139
hid_events.codes.BTN_SELECT = 0x13a
hid_events.codes.BTN_START = 0x13b
hid_events.codes.BTN_MODE = 0x13c
hid_events.codes.BTN_THUMBL = 0x13d
hid_events.codes.BTN_THUMBR = 0x13e

hid_events.codes.BTN_DIGI = 0x140
hid_events.codes.BTN_TOOL_PEN = 0x140
hid_events.codes.BTN_TOOL_RUBBER = 0x141
hid_events.codes.BTN_TOOL_BRUSH = 0x142
hid_events.codes.BTN_TOOL_PENCIL = 0x143
hid_events.codes.BTN_TOOL_AIRBRUSH = 0x144
hid_events.codes.BTN_TOOL_FINGER = 0x145
hid_events.codes.BTN_TOOL_MOUSE = 0x146
hid_events.codes.BTN_TOOL_LENS = 0x147
hid_events.codes.BTN_TOOL_QUINTTAP = 0x148
hid_events.codes.BTN_STYLUS3 = 0x149
hid_events.codes.BTN_TOUCH = 0x14a
hid_events.codes.BTN_STYLUS = 0x14b
hid_events.codes.BTN_STYLUS2 = 0x14c
hid_events.codes.BTN_TOOL_DOUBLETAP = 0x14d
hid_events.codes.BTN_TOOL_TRIPLETAP = 0x14e
hid_events.codes.BTN_TOOL_QUADTAP = 0x14f

hid_events.codes.BTN_WHEEL = 0x150
hid_events.codes.BTN_GEAR_DOWN = 0x150
hid_events.codes.BTN_GEAR_UP = 0x151

hid_events.codes.KEY_OK = 0x160
hid_events.codes.KEY_SELECT = 0x161
hid_events.codes.KEY_GOTO = 0x162
hid_events.codes.KEY_CLEAR = 0x163
hid_events.codes.KEY_POWER2 = 0x164
hid_events.codes.KEY_OPTION = 0x165
hid_events.codes.KEY_INFO = 0x166
hid_events.codes.KEY_TIME = 0x167
hid_events.codes.KEY_VENDOR = 0x168
hid_events.codes.KEY_ARCHIVE = 0x169
hid_events.codes.KEY_PROGRAM = 0x16a
hid_events.codes.KEY_CHANNEL = 0x16b
hid_events.codes.KEY_FAVORITES = 0x16c
hid_events.codes.KEY_EPG = 0x16d
hid_events.codes.KEY_PVR = 0x16e
hid_events.codes.KEY_MHP = 0x16f
hid_events.codes.KEY_LANGUAGE = 0x170
hid_events.codes.KEY_TITLE = 0x171
hid_events.codes.KEY_SUBTITLE = 0x172
hid_events.codes.KEY_ANGLE = 0x173
hid_events.codes.KEY_ZOOM = 0x174
hid_events.codes.KEY_MODE = 0x175
hid_events.codes.KEY_KEYBOARD = 0x176
hid_events.codes.KEY_SCREEN = 0x177
hid_events.codes.KEY_PC = 0x178
hid_events.codes.KEY_TV = 0x179
hid_events.codes.KEY_TV2 = 0x17a
hid_events.codes.KEY_VCR = 0x17b
hid_events.codes.KEY_VCR2 = 0x17c
hid_events.codes.KEY_SAT = 0x17d
hid_events.codes.KEY_SAT2 = 0x17e
hid_events.codes.KEY_CD = 0x17f
hid_events.codes.KEY_TAPE = 0x180
hid_events.codes.KEY_RADIO = 0x181
hid_events.codes.KEY_TUNER = 0x182
hid_events.codes.KEY_PLAYER = 0x183
hid_events.codes.KEY_TEXT = 0x184
hid_events.codes.KEY_DVD = 0x185
hid_events.codes.KEY_AUX = 0x186
hid_events.codes.KEY_MP3 = 0x187
hid_events.codes.KEY_AUDIO = 0x188
hid_events.codes.KEY_VIDEO = 0x189
hid_events.codes.KEY_DIRECTORY = 0x18a
hid_events.codes.KEY_LIST = 0x18b
hid_events.codes.KEY_MEMO = 0x18c
hid_events.codes.KEY_CALENDAR = 0x18d
hid_events.codes.KEY_RED = 0x18e
hid_events.codes.KEY_GREEN = 0x18f
hid_events.codes.KEY_YELLOW = 0x190
hid_events.codes.KEY_BLUE = 0x191
hid_events.codes.KEY_CHANNELUP = 0x192
hid_events.codes.KEY_CHANNELDOWN = 0x193
hid_events.codes.KEY_FIRST = 0x194
hid_events.codes.KEY_LAST = 0x195
hid_events.codes.KEY_AB = 0x196
hid_events.codes.KEY_NEXT = 0x197
hid_events.codes.KEY_RESTART = 0x198
hid_events.codes.KEY_SLOW = 0x199
hid_events.codes.KEY_SHUFFLE = 0x19a
hid_events.codes.KEY_BREAK = 0x19b
hid_events.codes.KEY_PREVIOUS = 0x19c
hid_events.codes.KEY_DIGITS = 0x19d
hid_events.codes.KEY_TEEN = 0x19e
hid_events.codes.KEY_TWEN = 0x19f
hid_events.codes.KEY_VIDEOPHONE = 0x1a0
hid_events.codes.KEY_GAMES = 0x1a1
hid_events.codes.KEY_ZOOMIN = 0x1a2
hid_events.codes.KEY_ZOOMOUT = 0x1a3
hid_events.codes.KEY_ZOOMRESET = 0x1a4
hid_events.codes.KEY_WORDPROCESSOR = 0x1a5
hid_events.codes.KEY_EDITOR = 0x1a6
hid_events.codes.KEY_SPREADSHEET = 0x1a7
hid_events.codes.KEY_GRAPHICSEDITOR = 0x1a8
hid_events.codes.KEY_PRESENTATION = 0x1a9
hid_events.codes.KEY_DATABASE = 0x1aa
hid_events.codes.KEY_NEWS = 0x1ab
hid_events.codes.KEY_VOICEMAIL = 0x1ac
hid_events.codes.KEY_ADDRESSBOOK = 0x1ad
hid_events.codes.KEY_MESSENGER = 0x1ae
hid_events.codes.KEY_DISPLAYTOGGLE = 0x1af
hid_events.codes.KEY_BRIGHTNESS_TOGGLE = hid_events.codes.KEY_DISPLAYTOGGLE
hid_events.codes.KEY_SPELLCHECK = 0x1b0
hid_events.codes.KEY_LOGOFF = 0x1b1

hid_events.codes.KEY_DOLLAR = 0x1b2
hid_events.codes.KEY_EURO = 0x1b3

hid_events.codes.KEY_FRAMEBACK = 0x1b4
hid_events.codes.KEY_FRAMEFORWARD = 0x1b5
hid_events.codes.KEY_CONTEXT_MENU = 0x1b6
hid_events.codes.KEY_MEDIA_REPEAT = 0x1b7
hid_events.codes.KEY_10CHANNELSUP = 0x1b8
hid_events.codes.KEY_10CHANNELSDOWN = 0x1b9
hid_events.codes.KEY_IMAGES = 0x1ba

hid_events.codes.KEY_DEL_EOL = 0x1c0
hid_events.codes.KEY_DEL_EOS = 0x1c1
hid_events.codes.KEY_INS_LINE = 0x1c2
hid_events.codes.KEY_DEL_LINE = 0x1c3

hid_events.codes.KEY_FN = 0x1d0
hid_events.codes.KEY_FN_ESC = 0x1d1
hid_events.codes.KEY_FN_F1 = 0x1d2
hid_events.codes.KEY_FN_F2 = 0x1d3
hid_events.codes.KEY_FN_F3 = 0x1d4
hid_events.codes.KEY_FN_F4 = 0x1d5
hid_events.codes.KEY_FN_F5 = 0x1d6
hid_events.codes.KEY_FN_F6 = 0x1d7
hid_events.codes.KEY_FN_F7 = 0x1d8
hid_events.codes.KEY_FN_F8 = 0x1d9
hid_events.codes.KEY_FN_F9 = 0x1da
hid_events.codes.KEY_FN_F10 = 0x1db
hid_events.codes.KEY_FN_F11 = 0x1dc
hid_events.codes.KEY_FN_F12 = 0x1dd
hid_events.codes.KEY_FN_1 = 0x1de
hid_events.codes.KEY_FN_2 = 0x1df
hid_events.codes.KEY_FN_D = 0x1e0
hid_events.codes.KEY_FN_E = 0x1e1
hid_events.codes.KEY_FN_F = 0x1e2
hid_events.codes.KEY_FN_S = 0x1e3
hid_events.codes.KEY_FN_B = 0x1e4

hid_events.codes.KEY_BRL_DOT1 = 0x1f1
hid_events.codes.KEY_BRL_DOT2 = 0x1f2
hid_events.codes.KEY_BRL_DOT3 = 0x1f3
hid_events.codes.KEY_BRL_DOT4 = 0x1f4
hid_events.codes.KEY_BRL_DOT5 = 0x1f5
hid_events.codes.KEY_BRL_DOT6 = 0x1f6
hid_events.codes.KEY_BRL_DOT7 = 0x1f7
hid_events.codes.KEY_BRL_DOT8 = 0x1f8
hid_events.codes.KEY_BRL_DOT9 = 0x1f9
hid_events.codes.KEY_BRL_DOT10 = 0x1fa

hid_events.codes.KEY_NUMERIC_0 = 0x200
hid_events.codes.KEY_NUMERIC_1 = 0x201
hid_events.codes.KEY_NUMERIC_2 = 0x202
hid_events.codes.KEY_NUMERIC_3 = 0x203
hid_events.codes.KEY_NUMERIC_4 = 0x204
hid_events.codes.KEY_NUMERIC_5 = 0x205
hid_events.codes.KEY_NUMERIC_6 = 0x206
hid_events.codes.KEY_NUMERIC_7 = 0x207
hid_events.codes.KEY_NUMERIC_8 = 0x208
hid_events.codes.KEY_NUMERIC_9 = 0x209
hid_events.codes.KEY_NUMERIC_STAR = 0x20a
hid_events.codes.KEY_NUMERIC_POUND = 0x20b
hid_events.codes.KEY_NUMERIC_A = 0x20c
hid_events.codes.KEY_NUMERIC_B = 0x20d
hid_events.codes.KEY_NUMERIC_C = 0x20e
hid_events.codes.KEY_NUMERIC_D = 0x20f

hid_events.codes.KEY_CAMERA_FOCUS = 0x210
hid_events.codes.KEY_WPS_BUTTON = 0x211

hid_events.codes.KEY_TOUCHPAD_TOGGLE = 0x212
hid_events.codes.KEY_TOUCHPAD_ON = 0x213
hid_events.codes.KEY_TOUCHPAD_OFF = 0x214

hid_events.codes.KEY_CAMERA_ZOOMIN = 0x215
hid_events.codes.KEY_CAMERA_ZOOMOUT = 0x216
hid_events.codes.KEY_CAMERA_UP = 0x217
hid_events.codes.KEY_CAMERA_DOWN = 0x218
hid_events.codes.KEY_CAMERA_LEFT = 0x219
hid_events.codes.KEY_CAMERA_RIGHT = 0x21a

hid_events.codes.KEY_ATTENDANT_ON = 0x21b
hid_events.codes.KEY_ATTENDANT_OFF = 0x21c
hid_events.codes.KEY_ATTENDANT_TOGGLE = 0x21d
hid_events.codes.KEY_LIGHTS_TOGGLE = 0x21e

hid_events.codes.BTN_DPAD_UP = 0x220
hid_events.codes.BTN_DPAD_DOWN = 0x221
hid_events.codes.BTN_DPAD_LEFT = 0x222
hid_events.codes.BTN_DPAD_RIGHT = 0x223

hid_events.codes.KEY_ALS_TOGGLE = 0x230

hid_events.codes.KEY_BUTTONCONFIG = 0x240
hid_events.codes.KEY_TASKMANAGER = 0x241
hid_events.codes.KEY_JOURNAL = 0x242
hid_events.codes.KEY_CONTROLPANEL = 0x243
hid_events.codes.KEY_APPSELECT = 0x244
hid_events.codes.KEY_SCREENSAVER = 0x245
hid_events.codes.KEY_VOICECOMMAND = 0x246
hid_events.codes.KEY_ASSISTANT = 0x247

hid_events.codes.KEY_BRIGHTNESS_MIN = 0x250
hid_events.codes.KEY_BRIGHTNESS_MAX = 0x251

hid_events.codes.KEY_KBDINPUTASSIST_PREV = 0x260
hid_events.codes.KEY_KBDINPUTASSIST_NEXT = 0x261
hid_events.codes.KEY_KBDINPUTASSIST_PREVGROUP = 0x262
hid_events.codes.KEY_KBDINPUTASSIST_NEXTGROUP = 0x263
hid_events.codes.KEY_KBDINPUTASSIST_ACCEPT = 0x264
hid_events.codes.KEY_KBDINPUTASSIST_CANCEL = 0x265

hid_events.codes.KEY_RIGHT_UP = 0x266
hid_events.codes.KEY_RIGHT_DOWN = 0x267
hid_events.codes.KEY_LEFT_UP = 0x268
hid_events.codes.KEY_LEFT_DOWN = 0x269

hid_events.codes.KEY_ROOT_MENU = 0x26a
hid_events.codes.KEY_MEDIA_TOP_MENU = 0x26b
hid_events.codes.KEY_NUMERIC_11 = 0x26c
hid_events.codes.KEY_NUMERIC_12 = 0x26d

hid_events.codes.KEY_AUDIO_DESC = 0x26e
hid_events.codes.KEY_3D_MODE = 0x26f
hid_events.codes.KEY_NEXT_FAVORITE = 0x270
hid_events.codes.KEY_STOP_RECORD = 0x271
hid_events.codes.KEY_PAUSE_RECORD = 0x272
hid_events.codes.KEY_VOD = 0x273
hid_events.codes.KEY_UNMUTE = 0x274
hid_events.codes.KEY_FASTREVERSE = 0x275
hid_events.codes.KEY_SLOWREVERSE = 0x276

hid_events.codes.KEY_DATA = 0x277
hid_events.codes.KEY_ONSCREEN_KEYBOARD = 0x278

hid_events.codes.BTN_TRIGGER_HAPPY = 0x2c0
hid_events.codes.BTN_TRIGGER_HAPPY1 = 0x2c0
hid_events.codes.BTN_TRIGGER_HAPPY2 = 0x2c1
hid_events.codes.BTN_TRIGGER_HAPPY3 = 0x2c2
hid_events.codes.BTN_TRIGGER_HAPPY4 = 0x2c3
hid_events.codes.BTN_TRIGGER_HAPPY5 = 0x2c4
hid_events.codes.BTN_TRIGGER_HAPPY6 = 0x2c5
hid_events.codes.BTN_TRIGGER_HAPPY7 = 0x2c6
hid_events.codes.BTN_TRIGGER_HAPPY8 = 0x2c7
hid_events.codes.BTN_TRIGGER_HAPPY9 = 0x2c8
hid_events.codes.BTN_TRIGGER_HAPPY10 = 0x2c9
hid_events.codes.BTN_TRIGGER_HAPPY11 = 0x2ca
hid_events.codes.BTN_TRIGGER_HAPPY12 = 0x2cb
hid_events.codes.BTN_TRIGGER_HAPPY13 = 0x2cc
hid_events.codes.BTN_TRIGGER_HAPPY14 = 0x2cd
hid_events.codes.BTN_TRIGGER_HAPPY15 = 0x2ce
hid_events.codes.BTN_TRIGGER_HAPPY16 = 0x2cf
hid_events.codes.BTN_TRIGGER_HAPPY17 = 0x2d0
hid_events.codes.BTN_TRIGGER_HAPPY18 = 0x2d1
hid_events.codes.BTN_TRIGGER_HAPPY19 = 0x2d2
hid_events.codes.BTN_TRIGGER_HAPPY20 = 0x2d3
hid_events.codes.BTN_TRIGGER_HAPPY21 = 0x2d4
hid_events.codes.BTN_TRIGGER_HAPPY22 = 0x2d5
hid_events.codes.BTN_TRIGGER_HAPPY23 = 0x2d6
hid_events.codes.BTN_TRIGGER_HAPPY24 = 0x2d7
hid_events.codes.BTN_TRIGGER_HAPPY25 = 0x2d8
hid_events.codes.BTN_TRIGGER_HAPPY26 = 0x2d9
hid_events.codes.BTN_TRIGGER_HAPPY27 = 0x2da
hid_events.codes.BTN_TRIGGER_HAPPY28 = 0x2db
hid_events.codes.BTN_TRIGGER_HAPPY29 = 0x2dc
hid_events.codes.BTN_TRIGGER_HAPPY30 = 0x2dd
hid_events.codes.BTN_TRIGGER_HAPPY31 = 0x2de
hid_events.codes.BTN_TRIGGER_HAPPY32 = 0x2df
hid_events.codes.BTN_TRIGGER_HAPPY33 = 0x2e0
hid_events.codes.BTN_TRIGGER_HAPPY34 = 0x2e1
hid_events.codes.BTN_TRIGGER_HAPPY35 = 0x2e2
hid_events.codes.BTN_TRIGGER_HAPPY36 = 0x2e3
hid_events.codes.BTN_TRIGGER_HAPPY37 = 0x2e4
hid_events.codes.BTN_TRIGGER_HAPPY38 = 0x2e5
hid_events.codes.BTN_TRIGGER_HAPPY39 = 0x2e6
hid_events.codes.BTN_TRIGGER_HAPPY40 = 0x2e7

hid_events.codes.KEY_MIN_INTERESTING = hid_events.codes.KEY_MUTE

-- Relative axes

hid_events.codes.REL_X = 0x00
hid_events.codes.REL_Y = 0x01
hid_events.codes.REL_Z = 0x02
hid_events.codes.REL_RX = 0x03
hid_events.codes.REL_RY = 0x04
hid_events.codes.REL_RZ = 0x05
hid_events.codes.REL_HWHEEL = 0x06
hid_events.codes.REL_DIAL = 0x07
hid_events.codes.REL_WHEEL = 0x08
hid_events.codes.REL_MISC = 0x09

-- Absolute axes

hid_events.codes.ABS_X = 0x00
hid_events.codes.ABS_Y = 0x01
hid_events.codes.ABS_Z = 0x02
hid_events.codes.ABS_RX = 0x03
hid_events.codes.ABS_RY = 0x04
hid_events.codes.ABS_RZ = 0x05
hid_events.codes.ABS_THROTTLE = 0x06
hid_events.codes.ABS_RUDDER = 0x07
hid_events.codes.ABS_WHEEL = 0x08
hid_events.codes.ABS_GAS = 0x09
hid_events.codes.ABS_BRAKE = 0x0a
hid_events.codes.ABS_HAT0X = 0x10
hid_events.codes.ABS_HAT0Y = 0x11
hid_events.codes.ABS_HAT1X = 0x12
hid_events.codes.ABS_HAT1Y = 0x13
hid_events.codes.ABS_HAT2X = 0x14
hid_events.codes.ABS_HAT2Y = 0x15
hid_events.codes.ABS_HAT3X = 0x16
hid_events.codes.ABS_HAT3Y = 0x17
hid_events.codes.ABS_PRESSURE = 0x18
hid_events.codes.ABS_DISTANCE = 0x19
hid_events.codes.ABS_TILT_X = 0x1a
hid_events.codes.ABS_TILT_Y = 0x1b
hid_events.codes.ABS_TOOL_WIDTH = 0x1c

hid_events.codes.ABS_VOLUME = 0x20

hid_events.codes.ABS_MISC = 0x28

hid_events.codes.ABS_MT_SLOT = 0x2f
hid_events.codes.ABS_MT_TOUCH_MAJOR = 0x30
hid_events.codes.ABS_MT_TOUCH_MINOR = 0x31
hid_events.codes.ABS_MT_WIDTH_MAJOR = 0x32
hid_events.codes.ABS_MT_WIDTH_MINOR = 0x33
hid_events.codes.ABS_MT_ORIENTATION = 0x34
hid_events.codes.ABS_MT_POSITION_X = 0x35
hid_events.codes.ABS_MT_POSITION_Y = 0x36
hid_events.codes.ABS_MT_TOOL_TYPE = 0x37
hid_events.codes.ABS_MT_BLOB_ID = 0x38
hid_events.codes.ABS_MT_TRACKING_ID = 0x39
hid_events.codes.ABS_MT_PRESSURE = 0x3a
hid_events.codes.ABS_MT_DISTANCE = 0x3b
hid_events.codes.ABS_MT_TOOL_X = 0x3c
hid_events.codes.ABS_MT_TOOL_Y = 0x3d

-- Switch events

hid_events.codes.SW_LID = 0x00
hid_events.codes.SW_TABLET_MODE = 0x01
hid_events.codes.SW_HEADPHONE_INSERT = 0x02
hid_events.codes.SW_RFKILL_ALL = 0x03
hid_events.codes.SW_RADIO = hid_events.codes.SW_RFKILL_ALL
hid_events.codes.SW_MICROPHONE_INSERT = 0x04
hid_events.codes.SW_DOCK = 0x05
hid_events.codes.SW_LINEOUT_INSERT = 0x06
hid_events.codes.SW_JACK_PHYSICAL_INSERT = 0x07
hid_events.codes.SW_VIDEOOUT_INSERT = 0x08
hid_events.codes.SW_CAMERA_LENS_COVER = 0x09
hid_events.codes.SW_KEYPAD_SLIDE = 0x0a
hid_events.codes.SW_FRONT_PROXIMITY = 0x0b
hid_events.codes.SW_ROTATE_LOCK = 0x0c
hid_events.codes.SW_LINEIN_INSERT = 0x0d
hid_events.codes.SW_MUTE_DEVICE = 0x0e
hid_events.codes.SW_PEN_INSERTED = 0x0f

-- Misc events

hid_events.codes.MSC_SERIAL = 0x00
hid_events.codes.MSC_PULSELED = 0x01
hid_events.codes.MSC_GESTURE = 0x02
hid_events.codes.MSC_RAW = 0x03
hid_events.codes.MSC_SCAN = 0x04
hid_events.codes.MSC_TIMESTAMP = 0x05

-- LEDs

hid_events.codes.LED_NUML = 0x00
hid_events.codes.LED_CAPSL = 0x01
hid_events.codes.LED_SCROLLL = 0x02
hid_events.codes.LED_COMPOSE = 0x03
hid_events.codes.LED_KANA = 0x04
hid_events.codes.LED_SLEEP = 0x05
hid_events.codes.LED_SUSPEND = 0x06
hid_events.codes.LED_MUTE = 0x07
hid_events.codes.LED_MISC = 0x08
hid_events.codes.LED_MAIL = 0x09
hid_events.codes.LED_CHARGING = 0x0a

-- Autorepeat values

hid_events.codes.REP_DELAY = 0x00
hid_events.codes.REP_PERIOD = 0x01

-- Sounds

hid_events.codes.SND_CLICK = 0x00
hid_events.codes.SND_BELL = 0x01
hid_events.codes.SND_TONE = 0x02

return hid_events
