--- keyboard (typing, not piano)
-- @module keyboard

local tab = require 'tabutil'
local te_kbd_cb = require 'lib/textentry_kbd'

local char_modifier = require 'core/keymap/char_modifier'

keyboard = {}

keyboard.keymap = {}
keyboard.keymap.us = require 'core/keymap/us'
keyboard.keymap.be = require 'core/keymap/be'
keyboard.keymap.hr = require 'core/keymap/hr'
keyboard.keymap.cz = require 'core/keymap/cz'
keyboard.keymap.dk = require 'core/keymap/dk'
keyboard.keymap.ee = require 'core/keymap/ee'
keyboard.keymap.fi = require 'core/keymap/fi'
keyboard.keymap.fr = require 'core/keymap/fr'
keyboard.keymap.de = require 'core/keymap/de'
keyboard.keymap.gr = require 'core/keymap/gr'
keyboard.keymap.hu = require 'core/keymap/hu'
keyboard.keymap.is = require 'core/keymap/is'
keyboard.keymap.ie = require 'core/keymap/ie'
keyboard.keymap.it = require 'core/keymap/it'
keyboard.keymap.jp = require 'core/keymap/jp'
keyboard.keymap.kp = require 'core/keymap/kp'
keyboard.keymap.lv = require 'core/keymap/lv'
keyboard.keymap.no = require 'core/keymap/no'
keyboard.keymap.pl = require 'core/keymap/pl'
keyboard.keymap.pt = require 'core/keymap/pt'
keyboard.keymap.ro = require 'core/keymap/ro'
keyboard.keymap.ru = require 'core/keymap/ru'
keyboard.keymap.rs = require 'core/keymap/rs'
keyboard.keymap.sk = require 'core/keymap/sk'
keyboard.keymap.si = require 'core/keymap/si'
keyboard.keymap.es = require 'core/keymap/es'
keyboard.keymap.se = require 'core/keymap/se'
keyboard.keymap.uk = require 'core/keymap/uk'
keyboard.selected_map = "us"

local km = keyboard.keymap[keyboard.selected_map]

function keyboard.load_enabled()
  local m_fn, err = loadfile(_path.keyboard_layout)
  if err or m_fn == nil or m_fn() == nil then
    -- NB: curently keeping current value
    print("error loading keyboard layout, using old value: "..keyboard.selected_map)
    -- could also fall back to US w/:
    -- keyboard.selected_map = "us"
  else
    keyboard.selected_map = m_fn()
    print("loaded keyboard layout: "..keyboard.selected_map)
  end
  km = keyboard.keymap[keyboard.selected_map]
end

function keyboard.save_enabled()
  local file, err = io.open(_path.keyboard_layout, "wb")
  if err then return err end
  file:write("return '"..keyboard.selected_map.."'")
  file:close()
end

keyboard.load_enabled()

function keyboard.set_map(m, autosave)
  if keyboard.keymap[m] then
    keyboard.selected_map = m
    km = keyboard.keymap[keyboard.selected_map]
    print("loaded keyboard layout: "..keyboard.selected_map)
    if autosave then
      keyboard.save_enabled()
    end
  end
end

local function km_uses_altgr()
  return km[char_modifier.ALTGR] ~= nil
    or km[char_modifier.SHIFT | char_modifier.ALTGR] ~= nil
end

--- key states
keyboard.state = {}

-- clear callbacks
function keyboard.clear()
  keyboard.code = function() end
  keyboard.char = function() end
end

--- key code callback, script should redefine.
-- use this primarily for responding to physical key presses ('LEFTSHIFT',
-- 'ENTER', 'F7', etc) not printable characters, as it does not adjust to
-- the setting of the user's keyboard layout, including national variant.
-- if you want to detect the character, use @{char} or @{code_to_char}.
--
-- @tparam string key    a code representing the physical key pressed.
-- @tparam number value    0 = released, 1 = pressed, 2 = pressed and held.
--
function keyboard.code(key, value) end

--- key character callback, script should redefine.
-- will only be called for printable characters such as letters, numbers
-- and punctuation - and space. will take account of the setting of the user's keyboard
-- layout, including national variant.
--
-- @tparam string ch    the printable character intended to be generated.
--
function keyboard.char(ch) end


--- return SHIFT state
function keyboard.shift()
  return keyboard.state.LEFTSHIFT or keyboard.state.RIGHTSHIFT end
--- return ALT state
function keyboard.alt()
  return keyboard.state.LEFTALT or (not km_uses_altgr() and keyboard.state.RIGHTALT) end
function keyboard.altgr()
  return km_uses_altgr() and keyboard.state.RIGHTALT end
--- return CTRL state
function keyboard.ctrl()
  return keyboard.state.LEFTCTRL or keyboard.state.RIGHTCTRL end
--- return META state
function keyboard.meta()
  return keyboard.state.LEFTMETA or keyboard.state.RIGHTMETA end


function keyboard.process(type,code,value)
  local c = keyboard.codes[code]
  if c == nil then
    return
  end

  -- textentry keycode
  if te_kbd_cb.code then
    te_kbd_cb.code(c,value)
    -- menu screen goto
  elseif (c=="F1" or c=="F2" or c=="F3" or c=="F4") and value==1 then 
    _menu.set_mode(true)
    _menu.keycode(c,value)
    -- toggle menu with F5
  elseif (c=="F5" and value==1) then 
    _menu.set_mode(not _menu.mode)
    -- menu keycode
  elseif _menu.mode then _menu.keycode(c,value)
    -- script keycode
  elseif keyboard.code then keyboard.code(c,value) end

  keyboard.state[c] = value>0

  local a = keyboard.code_to_char(c)

  if value>0 and a then
    --print("char: "..a)
    -- menu keychar
    if te_kbd_cb.char then te_kbd_cb.char(a)
      -- script keychar
    elseif _menu.mode then _menu.keychar(a)
      -- textentry keycode
    elseif keyboard.char then keyboard.char(a) end
  end
  --print("kb",code,value,keyboard.codes[code])
end

--- convert a keyboard code to a printable character.
-- the output will be based on the setting of they user's keyboard
-- layout and the state of the modifier keys currently pressed
-- (eg a 'SHIFT' key).
--
-- @tparam string code    a code representing a physical key,
--     as passed into @{code}.
-- @treturn string    a string, or nil if no conversion is possible.
--
function keyboard.code_to_char(code)
  if code == nil then return nil end

  local c_mods = char_modifier.NONE
  if keyboard.shift() then c_mods = c_mods | char_modifier.SHIFT end
  if keyboard.altgr() then c_mods = c_mods | char_modifier.ALTGR end

  return km[c_mods][code]
end

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

keyboard.state = tab.invert(keyboard.codes)
for k,_ in pairs(keyboard.state) do keyboard.state[k] = false end

return keyboard
