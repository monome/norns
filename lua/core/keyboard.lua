keyboard = {}

keyboard.keymap = {}
keyboard.keymap.us = require 'core/keymap/us' 
keyboard.selected_map = "us"

local km = keyboard.keymap[keyboard.selected_map]


keyboard.state = {
  Lshift = false,
  Rshift = false,
  Lalt = false,
  Ralt = false,
  Lctrl = false,
  Rctrl = false,
  Lmeta = false,
  Rmeta = false
}

function keyboard.clear()
  keyboard.code = nil      -- callback for code keyboard codes
  keyboard.ascii = nil    -- callback for ascii keycodes, by map
end

function keyboard.set_map(m)
  if keyboard.keymap[m] then
    keyboard.selected_map = m
  end
end

function keyboard.shift()
  return keyboard.state.Lshift or keyboard.state.Rshift end
function keyboard.alt()
  return keyboard.state.Lalt or keyboard.state.Ralt end
function keyboard.ctrl()
  return keyboard.state.Lctrl or keyboard.state.Rctrl end
function keyboard.meta()
  return keyboard.state.Lmeta or keyboard.state.Rmeta end


function keyboard.process(type,code,value)
  if keyboard.code then
    keyboard.code(keyboard.codes[code],value)
  end

  local c = keyboard.codes[code] 
  if keyboard.process_mods[c] then
    keyboard.process_mods[c](value)
  end

  if value>0 then
    local a = km[keyboard.shift()][c]
    if a then 
      if keyboard.ascii then
        keyboard.ascii(a)
      end
      --print("ASCII: "..a)
    end
  end

  --print("kb",code,value,keyboard.codes[code])
end

keyboard.process_mods = {}
keyboard.process_mods.LEFTSHIFT = function(value) keyboard.state.Lshift = (value>0) end
keyboard.process_mods.RIGHTSHIFT = function(value) keyboard.state.Rshift = (value>0) end
keyboard.process_mods.LEFTALT = function(value) keyboard.state.Lalt = (value>0) end
keyboard.process_mods.RIGHTALT = function(value) keyboard.state.Ralt = (value>0) end
keyboard.process_mods.LEFTCTRL = function(value) keyboard.state.Lctrl = (value>0) end
keyboard.process_mods.RIGHTSHIFT = function(value) keyboard.state.Rctrl = (value>0) end
keyboard.process_mods.LEFTMETA = function(value) keyboard.state.Lmeta = (value>0) end
keyboard.process_mods.RIGHTMETA= function(value) keyboard.state.Rmeta = (value>0) end

keyboard.codes = {}

keyboard.codes[0] = 'RESERVED'
keyboard.codes[1] = 'ESC'
keyboard.codes[2] = '1'
keyboard.codes[3] = '2'
keyboard.codes[4] = '3'
keyboard.codes[5] = '4'
keyboard.codes[6] = '5'
keyboard.codes[7] = '6'
keyboard.codes[8] = '7'
keyboard.codes[9] = '8'
keyboard.codes[10] = '9'
keyboard.codes[11] = '0'
keyboard.codes[12] = 'MINUS'
keyboard.codes[13] = 'EQUAL'
keyboard.codes[14] = 'BACKSPACE'
keyboard.codes[15] = 'TAB'
keyboard.codes[16] = 'Q'
keyboard.codes[17] = 'W'
keyboard.codes[18] = 'E'
keyboard.codes[19] = 'R'
keyboard.codes[20] = 'T'
keyboard.codes[21] = 'Y'
keyboard.codes[22] = 'U'
keyboard.codes[23] = 'I'
keyboard.codes[24] = 'O'
keyboard.codes[25] = 'P'
keyboard.codes[26] = 'LEFTBRACE'
keyboard.codes[27] = 'RIGHTBRACE'
keyboard.codes[28] = 'ENTER'
keyboard.codes[29] = 'LEFTCTRL'
keyboard.codes[30] = 'A'
keyboard.codes[31] = 'S'
keyboard.codes[32] = 'D'
keyboard.codes[33] = 'F'
keyboard.codes[34] = 'G'
keyboard.codes[35] = 'H'
keyboard.codes[36] = 'J'
keyboard.codes[37] = 'K'
keyboard.codes[38] = 'L'
keyboard.codes[39] = 'SEMICOLON'
keyboard.codes[40] = 'APOSTROPHE'
keyboard.codes[41] = 'GRAVE'
keyboard.codes[42] = 'LEFTSHIFT'
keyboard.codes[43] = 'BACKSLASH'
keyboard.codes[44] = 'Z'
keyboard.codes[45] = 'X'
keyboard.codes[46] = 'C'
keyboard.codes[47] = 'V'
keyboard.codes[48] = 'B'
keyboard.codes[49] = 'N'
keyboard.codes[50] = 'M'
keyboard.codes[51] = 'COMMA'
keyboard.codes[52] = 'DOT'
keyboard.codes[53] = 'SLASH'
keyboard.codes[54] = 'RIGHTSHIFT'
keyboard.codes[55] = 'KPASTERISK'
keyboard.codes[56] = 'LEFTALT'
keyboard.codes[57] = 'SPACE'
keyboard.codes[58] = 'CAPSLOCK'
keyboard.codes[59] = 'F1'
keyboard.codes[60] = 'F2'
keyboard.codes[61] = 'F3'
keyboard.codes[62] = 'F4'
keyboard.codes[63] = 'F5'
keyboard.codes[64] = 'F6'
keyboard.codes[65] = 'F7'
keyboard.codes[66] = 'F8'
keyboard.codes[67] = 'F9'
keyboard.codes[68] = 'F10'
keyboard.codes[69] = 'NUMLOCK'
keyboard.codes[70] = 'SCROLLLOCK'
keyboard.codes[71] = 'KP7'
keyboard.codes[72] = 'KP8'
keyboard.codes[73] = 'KP9'
keyboard.codes[74] = 'KPMINUS'
keyboard.codes[75] = 'KP4'
keyboard.codes[76] = 'KP5'
keyboard.codes[77] = 'KP6'
keyboard.codes[78] = 'KPPLUS'
keyboard.codes[79] = 'KP1'
keyboard.codes[80] = 'KP2'
keyboard.codes[81] = 'KP3'
keyboard.codes[82] = 'KP0'
keyboard.codes[83] = 'KPDOT'
keyboard.codes[85] = 'ZENKAKUHANKAKU'
keyboard.codes[86] = '102ND'
keyboard.codes[87] = 'F11'
keyboard.codes[88] = 'F12'
keyboard.codes[89] = 'RO'
keyboard.codes[90] = 'KATAKANA'
keyboard.codes[91] = 'HIRAGANA'
keyboard.codes[92] = 'HENKAN'
keyboard.codes[93] = 'KATAKANAHIRAGANA'
keyboard.codes[94] = 'MUHENKAN'
keyboard.codes[95] = 'KPJPCOMMA'
keyboard.codes[96] = 'KPENTER'
keyboard.codes[97] = 'RIGHTCTRL'
keyboard.codes[98] = 'KPSLASH'
keyboard.codes[99] = 'SYSRQ'
keyboard.codes[100] = 'RIGHTALT'
keyboard.codes[101] = 'LINEFEED'
keyboard.codes[102] = 'HOME'
keyboard.codes[103] = 'UP'
keyboard.codes[104] = 'PAGEUP'
keyboard.codes[105] = 'LEFT'
keyboard.codes[106] = 'RIGHT'
keyboard.codes[107] = 'END'
keyboard.codes[108] = 'DOWN'
keyboard.codes[109] = 'PAGEDOWN'
keyboard.codes[110] = 'INSERT'
keyboard.codes[111] = 'DELETE'
keyboard.codes[112] = 'MACRO'
keyboard.codes[113] = 'MUTE'
keyboard.codes[114] = 'VOLUMEDOWN'
keyboard.codes[115] = 'VOLUMEUP'
keyboard.codes[116] = 'POWER'
keyboard.codes[117] = 'KPEQUAL'
keyboard.codes[118] = 'KPPLUSMINUS'
keyboard.codes[119] = 'PAUSE'
keyboard.codes[120] = 'SCALE'
keyboard.codes[121] = 'KPCOMMA'
keyboard.codes[122] = 'HANGUEL'
keyboard.codes[123] = 'HANJA'
keyboard.codes[124] = 'YEN'
keyboard.codes[125] = 'LEFTMETA'
keyboard.codes[126] = 'RIGHTMETA'
keyboard.codes[127] = 'COMPOSE'


return keyboard

