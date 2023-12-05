--- Screen class
-- subset of cairo drawing functions. see https://www.cairographics.org/tutorial/
-- @module screen

local Screen = {}

local metro = require 'core/metro'
local screensaver = metro[36]

local sleeping = false

local executable_lua, err = loadfile(_path.display_settings)
local loaded_settings = executable_lua ~= nil and executable_lua() or {}
local brightness = loaded_settings.brightness or 15
local contrast = loaded_settings.contrast or 127
local gamma = loaded_settings.gamma or 1.0
local module_just_loaded = true

screensaver.event = function()
  _norns.screen_clear()
  _norns.screen_update()
  sleeping = true
  Screen.update = function() end
end
screensaver.time = 900
screensaver.count = 1

--- copy buffer to screen.
Screen.update_default = function()
  if module_just_loaded then
    _norns.screen_brightness(brightness)
    _norns.screen_contrast(contrast)
    _norns.screen_gamma(gamma)
    module_just_loaded = false
  end
  _norns.screen_update()
end

--- restart screen saver timer
Screen.ping = function()
  screensaver:start()
  if sleeping == true then
    Screen.update = Screen.update_default
  end
end

--- low battery screen update
Screen.update_low_battery = function()
  _norns.screen_rect(32,34,64,16)
  _norns.screen_level(0)
  _norns.screen_fill()
  _norns.screen_move(64,45)
  _norns.screen_level(15)
  _norns.screen_text_center("LOW BATTERY")
  _norns.screen_update()
end

Screen.update = Screen.update_default

--- enable/disable anti-aliasing.
-- @param state on(1) or off(0)
Screen.aa = function(state) _norns.screen_aa(state) end

--- clear.
Screen.clear = function() _norns.screen_clear() end

--- set level (color/brightness).
-- @tparam number value 0-15 (0=off, 15=white)
Screen.level = function(value) _norns.screen_level(value) end

--- set line width.
-- @tparam number w line width (in pixels, floats permitted)
Screen.line_width = function(w) _norns.screen_line_width(w) end

--- set line cap style.
-- @param style line cap style string ("butt", "round" or "square"). default is "butt".
Screen.line_cap = function(style)
  _norns.screen_line_cap(style)
end

--- set line join style.
-- @param style line join style string ("miter", "round" or "bevel"). default is "miter"
Screen.line_join = function(style)
  _norns.screen_line_join(style)
end

--- set miter limit.
-- @param limit if the current line join style is set to "miter", the miter limit is used to determine whether the lines should be joined with a bevel instead of a miter. if the length of the miter divided by the line width is greater than the miter limit, the style is converted to a bevel. default value 10.
Screen.miter_limit = function(limit)
  _norns.screen_miter_limit(limit)
end

--- move drawing position.
-- @tparam number x position x
-- @tparam number y position y
Screen.move = function(x, y) _norns.screen_move(x, y) end

--- move drawing position relative to current position.
-- @tparam number x relative position x
-- @tparam number y relative position y
Screen.move_rel = function(x, y) _norns.screen_move_rel(x, y) end

--- draw line to specified point.
-- @tparam number x destination x
-- @tparam number y destination y
Screen.line = function(x,y) _norns.screen_line(x,y) end

--- draw line to specified point relative to current position.
-- @tparam number x relative destination x
-- @tparam number y relative destination y
Screen.line_rel = function(x, y) _norns.screen_line_rel(x, y) end

--- draw arc.
-- @tparam number x circle center x
-- @tparam number y circle center y
-- @tparam number r radius
-- @tparam number angle1 start angle
-- @tparam number angle2 end angle
Screen.arc = function(x, y, r, angle1, angle2) _norns.screen_arc(x, y, r, angle1, angle2) end

--- draw circle.
-- @tparam number x origin x
-- @tparam number y origin y
-- @tparam number r radius
Screen.circle = function(x, y, r) _norns.screen_circle(x, y, r) end

--- draw rectangle.
-- @tparam number x x position
-- @tparam number y y position
-- @tparam number w width
-- @tparam number h height
Screen.rect = function(x, y, w, h) _norns.screen_rect(x, y, w, h) end

--- draw curve (cubic Bézier spline).
-- @tparam number x1 handle 1 x
-- @tparam number y1 handle 1 y
-- @tparam number x2 handle 2 x
-- @tparam number y2 handle 2 y
-- @tparam number x3 destination x
-- @tparam number y3 destination y
Screen.curve = function(x1, y1, x2, y2, x3, y3) _norns.screen_curve(x1, y1, x2, y2, x3, y3) end

--- draw curve (cubic Bézier spline) relative coordinates.
-- @tparam number x1 handle 1 x
-- @tparam number y1 handle 1 y
-- @tparam number x2 handle 2 x
-- @tparam number y2 handle 2 y
-- @tparam number x3 relative destination x
-- @tparam number y3 relative destination y
Screen.curve_rel = function(x1, y1, x2, y2, x3, y3) _norns.screen_curve_rel(x1, y1, x2, y2, x3, y3) end

--- close current path.
Screen.close = function() _norns.screen_close() end

--- stroke current path.
-- uses currently selected color.
-- after this call the current path will be cleared, so the 'relative' functions
-- (`move_rel`, `line_rel` and `curve_rel`) won't work - use their absolute
-- alternatives instead.
Screen.stroke = function() _norns.screen_stroke() end

--- fill current path.
-- uses currently selected color.
-- after this call the current path will be cleared, so the 'relative' functions
-- (`move_rel`, `line_rel` and `curve_rel`) won't work - use their absolute
-- alternatives instead.
Screen.fill = function() _norns.screen_fill() end

--- draw text (left aligned).
-- uses currently selected font.
-- @tparam string str : text to write
Screen.text = function(str) _norns.screen_text(str) end

--- draw left-aligned text, trimmed to specified width.
-- (characters are removed from end of string until it fits.)
-- uses currently selected font.
-- @tparam string str : text to write
-- @tparam number w: width 
Screen.text_trim = function(str, w) _norns.screen_text_trim(str, w) end

--- draw text (left aligned) and rotated.
-- uses currently selected font.
-- @tparam number x x position
-- @tparam number y y position
-- @tparam string str : text to write
-- @tparam number degrees : degrees to rotate
Screen.text_rotate = function(x, y, str, degrees) _norns.screen_text_rotate(x, y, str, degrees) end

--- draw text, right aligned.
-- uses currently selected font
-- @tparam string str : text to write.
Screen.text_right = function(str) _norns.screen_text_right(str) end

--- draw text, center aligned.
-- uses currently selected font.
-- @tparam string str : text to write
Screen.text_center = function(str) _norns.screen_text_center(str) end

--- draw text, center aligned, and rotated.
-- uses currently selected font.
-- @tparam number x x position
-- @tparam number y y position
-- @tparam string str : text to write
-- @tparam number degrees : degress to rotate
Screen.text_center_rotate = function(x, y, str, degrees) _norns.screen_text_center_rotate(x, y, str, degrees) end

--- calculate width of text given current font and draw state.
-- @tparam string str : text to calculate width of
Screen.text_extents = function(str) 
  return _norns.screen_text_extents(str)
end

-- get the current drawing position in the screen surface
-- @treturn number x
-- @treturn number y
Screen.current_point = function() return _norns.screen_current_point() end

--- select font face.
-- @param index font face (see list, or Screen.font_face_names)
--
-- 1 04B_03 (norns default)
-- 2 ALEPH
-- 3 Roboto Thin
-- 4 Roboto Light
-- 5 Roboto Regular
-- 6 Roboto Medium
-- 7 Roboto Bold
-- 8 Roboto Black
-- 9 Roboto Thin Italic
-- 10 Roboto Light Italic
-- 11 Roboto Italic
-- 12 Roboto Medium Italic
-- 13 Roboto Bold Italic
-- 14 Roboto Black Italic
-- 15 VeraBd
-- 16 VeraBI
-- 17 VeraIt
-- 18 VeraMoBd
-- 19 VeraMoBI
-- 20 VeraMoIt
-- 21 VeraMono
-- 22 VeraSeBd
-- 23 VeraSe
-- 24 Vera
-- 25 bmp/tom-thumb
-- 26 creep
-- 27 ctrld-fixed-10b
-- 28 ctrld-fixed-10r
-- 29 ctrld-fixed-13b
-- 30 ctrld-fixed-13b-i
-- 31 ctrld-fixed-13r
-- 32 ctrld-fixed-13r-i
-- 33 ctrld-fixed-16b
-- 34 ctrld-fixed-16b-i
-- 35 ctrld-fixed-16r
-- 36 ctrld-fixed-16r-i
-- 37 scientifica-11
-- 38 scientificaBold-11
-- 39 scientificaItalic-11
-- 40 ter-u12b
-- 41 ter-u12n
-- 42 ter-u14b
-- 43 ter-u14n
-- 44 ter-u14v
-- 45 ter-u16b
-- 46 ter-u16n
-- 47 ter-u16v
-- 48 ter-u18b
-- 49 ter-u18n
-- 50 ter-u20b
-- 51 ter-u20n
-- 52 ter-u22b
-- 53 ter-u22n
-- 54 ter-u24b
-- 55 ter-u24n
-- 56 ter-u28b
-- 57 ter-u28n
-- 58 ter-u32b
-- 59 ter-u32n
-- 60 unscii-16-full.pcf
-- 61 unscii-16.pcf
-- 62 unscii-8-alt.pcf
-- 63 unscii-8-fantasy.pcf
-- 64 unscii-8-mcr.pcf
-- 65 unscii-8.pcf
-- 66 unscii-8-tall.pcf
-- 67 unscii-8-thin.pcf
Screen.font_face = function(index) _norns.screen_font_face(index) end
Screen.font_face_count = 68
Screen.font_face_names = {
   "04B_03__",
   "liquid",
   "Roboto-Thin",
   "Roboto-Light",
   "Roboto-Regular",
   "Roboto-Medium",
   "Roboto-Bold",
   "Roboto-Black",
   "Roboto-ThinItalic",
   "Roboto-LightItalic",
   "Roboto-Italic",
   "Roboto-MediumItalic",
   "Roboto-BoldItalic",
   "Roboto-BlackItalic",
   "VeraBd",
   "VeraBI",
   "VeraIt",
   "VeraMoBd",
   "VeraMoBI",
   "VeraMoIt",
   "VeraMono",
   "VeraSeBd",
   "VeraSe",
   "Vera",
   "bmp/tom-thumb",
   "bmp/creep",
   "bmp/ctrld-fixed-10b",
   "bmp/ctrld-fixed-10r",
   "bmp/ctrld-fixed-13b",
   "bmp/ctrld-fixed-13b-i",
   "bmp/ctrld-fixed-13r",
   "bmp/ctrld-fixed-13r-i",
   "bmp/ctrld-fixed-16b",
   "bmp/ctrld-fixed-16b-i",
   "bmp/ctrld-fixed-16r",
   "bmp/ctrld-fixed-16r-i",
   "bmp/scientifica-11",
   "bmp/scientificaBold-11",
   "bmp/scientificaItalic-11",
   "bmp/ter-u12b",
   "bmp/ter-u12n",
   "bmp/ter-u14b",
   "bmp/ter-u14n",
   "bmp/ter-u14v",
   "bmp/ter-u16b",
   "bmp/ter-u16n",
   "bmp/ter-u16v",
   "bmp/ter-u18b",
   "bmp/ter-u18n",
   "bmp/ter-u20b",
   "bmp/ter-u20n",
   "bmp/ter-u22b",
   "bmp/ter-u22n",
   "bmp/ter-u24b",
   "bmp/ter-u24n",
   "bmp/ter-u28b",
   "bmp/ter-u28n",
   "bmp/ter-u32b",
   "bmp/ter-u32n",
   "bmp/unscii-16-full",
   "bmp/unscii-16",
   "bmp/unscii-8-alt",
   "bmp/unscii-8-fantasy",
   "bmp/unscii-8-mcr",
   "bmp/unscii-8",
   "bmp/unscii-8-tall",
   "bmp/unscii-8-thin",
   "Particle",
}

--- set font size.
-- @tparam number size in pixel height.
Screen.font_size = function(size) _norns.screen_font_size(size) end

--- draw single pixel (requires integer x/y, fill afterwards).
-- @tparam number x position
-- @tparam number y position
Screen.pixel = function(x, y)
  _norns.screen_rect(x, y, 1, 1)
end
_norns.screen_text_rotate = function(x, y, str, degrees)
  _norns.screen_save()
  _norns.screen_move(x, y)
  _norns.screen_translate(x, y)
  _norns.screen_rotate(util.degs_to_rads(degrees))
  _norns.screen_text(str)
  _norns.screen_restore()
end

_norns.screen_text_center_rotate = function(x, y, str, degrees)
  _norns.screen_save()
  _norns.screen_move(x, y)
  _norns.screen_translate(x, y)
  _norns.screen_rotate(util.degs_to_rads(degrees))
  _norns.screen_text_center(str)
  _norns.screen_restore()
end

_norns.screen_circle = function(x, y, r)
  _norns.screen_arc(x, y, r, 0, math.pi*2)
end

--- export screenshot
-- @param filename: saved to dust/data/(script)/(filename).png
Screen.export_screenshot = function(filename) _norns.screen_export_screenshot(norns.state.data..filename..'.png') end

--- display png
-- @param filename
-- @tparam number x x position
-- @tparam number y y position
Screen.display_png = function(filename, x, y) _norns.screen_display_png(filename, x, y) end

--- load png into an image buffer
-- @param filename
Screen.load_png = function(filename) return _norns.screen_load_png(filename) end

--- create an image buffer
-- @tparam number width image witdth
-- @tparam number height image height
Screen.create_image = function(width, height) return _norns.screen_create_image(width, height) end

--- display image buffer
-- @param image
-- @tparam number x x position
-- @tparam number y y position
Screen.display_image = function(image, x, y) _norns.screen_display_image(image, x, y) end

--- display sub-region image buffer
-- @param image
-- @tparam number left left inset within image
-- @tparam number top top inset within image
-- @tparam number width width from right within image
-- @tparam number height height from top within image
-- @tparam number x x position
-- @tparam number y y position
Screen.display_image_region = function(image, left, top, width, height, x, y)
  _norns.screen_display_image_region(image, left, top, width, height, x, y)
end

--- direct screen drawing within the provide function into the image instead of the screen
-- @tparam image image the image to draw into
-- @tparam function func function called to perform drawing
Screen.draw_to = function(image, func)
  image:_context_focus()
  local ok, result = pcall(func)
  image:_context_defocus()
  if not ok then print(result) else return result end
end

--- get a rectangle of screen content. returned buffer contains one byte (valued 0 - 15) per pixel, i.e. w * h bytes
-- @tparam number x x position
-- @tparam number y y position
-- @tparam number w width, default 1
-- @tparam number h height, default 1
Screen.peek = function(x, y, w, h)
  return _norns.screen_peek(x, y, w or 1, h or 1)
end

--- set a rectangle of screen content. expected buffer contains one byte (valued 0 - 15) per pixel, i.e. w * h bytes
-- @tparam number x x position
-- @tparam number y y position
-- @tparam number w width
-- @tparam number h height
-- @tparam string s screen content to set
Screen.poke = function(x, y, w, h, s) _norns.screen_poke(x, y, w, h, s) end

--- rotate
-- @tparam number radians
Screen.rotate = function(r) _norns.screen_rotate(r) end

--- move origin position
-- @tparam number x position x
-- @tparam number y position y
Screen.translate = function(x, y) _norns.screen_translate(x, y) end

--- save
Screen.save = function() _norns.screen_save() end

-- restore
Screen.restore = function() _norns.screen_restore() end

Screen.BLEND_MODES = {
  ['NONE'] = 0,
  ['DEFAULT'] = 0,
  ['OVER'] = 0,
  ['XOR'] = 1,
  ['ADD'] = 2,
  ['MULTIPLY'] = 3,
  ['SCREEN'] = 4,
  ['OVERLAY'] = 5,
  ['DARKEN'] = 6,
  ['LIGHTEN'] = 7,
  ['COLOR_DODGE'] = 8,
  ['COLOR_BURN'] = 9,
  ['HARD_LIGHT'] = 10,
  ['SOFT_LIGHT'] = 11,
  ['DIFFERENCE'] = 12,
  ['EXCLUSION'] = 13,
  ['CLEAR'] = 14,
  ['SOURCE'] = 15,
  ['IN'] = 16,
  ['OUT'] = 17,
  ['ATOP'] = 18,
  ['DEST'] = 19,
  ['DEST_OVER'] = 20,
  ['DEST_IN'] = 21,
  ['DEST_OUT'] = 22,
  ['DEST_ATOP'] = 23,
  ['SATURATE'] = 24,
  ['HSL_HUE'] = 25,
  ['HSL_SATURATION'] = 26,
  ['HSL_COLOR'] = 27,
  ['HSL_LUMINOSITY'] = 28,
}

--- change screen blending mode.
-- @tparam number/string index blending mode (see list), strings are case-insensitive, include '_' between words
--
-- more info at https://www.cairographics.org/operators/
--
-- there are other operators available, see the above link or use tab.print(screen.BLEND_MODES) in the REPL for the full list.
--
-- 0 Over (default)
--
-- 1 XOR: clears any overlapping pixels.
--
-- 2 Add: adds together the alpha (brightness) of overlapping pixels.
--
-- 3 Multiply: multiplies the colors of overlapping pixels, the result is always darker than the two inputs.
--
-- 4 Screen: the colors of overlapping pixels are complemented, multiplied, then their product is complimented. the result is always lighter than the two inputs.
--
-- 5 Overlay: multiplies colors if destination pixel level is >= 8, screens colors if destination pixel level is < 8.
--
-- 6 Darken: keeps the darker value of overlapping pixels.
--
-- 7 Lighten: keeps the lighter value of overlapping pixels.
--
-- 8 Color_Dodge: brightens pixels being drawn over.
--
-- 9 Color_Burn: darkens pixels being drawn over.
--
-- 10 Hard_Light: multiplies colors if source pixel level is >= 8, screens colors if source pixel level is < 8.
--
-- 11 Soft_Light: uses Darken or Lighten depending on the color of the source pixel.
--
-- 12 Difference: the result is the absolute value of the difference of the destination and source pixels.
--
-- 13 Exclusion: similar to Difference, but has lower contrast.
-- @usage -- number vs. string input
-- screen.blend_mode(0)
-- screen.blend_mode('over')
-- @usage -- case-insensitivity
-- screen.blend_mode('hard_light')
-- screen.blend_mode('hArD_lIgHt')
-- screen.blend_mode('HARD_LIGHT')

Screen.blend_mode = function(index)
  if type(index) == "string" then
    local i = Screen.BLEND_MODES[string.upper(index)]
    if i ~= nil then
      _norns.screen_set_operator(i)
    else
      print(index..' is not a valid blending mode, use tab.print(screen.BLEND_MODES) to see available modes and their indexes.')
    end
  elseif type(index) == "number" then
    _norns.screen_set_operator(index)
  end
end

--@tparam number/boolean i
--
-- if number:
-- i == 0 -> normal mode
-- i < 0 -> inverted mode
-- i > 0 -> inverted mode
--
-- if boolean:
-- false -> normal mode
-- true -> inverted mode
Screen.invert = function(i)
  if type(i) == "boolean" then
    _norns.screen_invert(i and 1 or 0)
  elseif type(i) == "number" then
    _norns.screen_invert(i == 0.0 and 0 or 1)
  end
end

Screen.sleep = function()
  screensaver:event()
end

return Screen
