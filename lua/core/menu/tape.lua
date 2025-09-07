local fileselect = require 'fileselect'
local textentry = require 'textentry'
local listselect = require 'listselect'
local util = require 'util'

local TAPE_MODE_PLAY = 1
local TAPE_MODE_REC = 2

local TAPE_PLAY_STATE_EMPTY = 0
local TAPE_PLAY_STATE_READY = 1
local TAPE_PLAY_STATE_PLAYING = 2
local TAPE_PLAY_STATE_PAUSED = 3

local TAPE_REC_STATE_EMPTY = 0
local TAPE_REC_STATE_READY = 1
local TAPE_REC_STATE_RECORDING = 2
local TAPE_REC_STATE_PAUSED = 3

local m = {
  mode = TAPE_MODE_PLAY,
  play = {
    state = TAPE_PLAY_STATE_EMPTY,
    file = nil,
    dir_prev = nil,
    pos_tick = 0,
    loop = true,
    needs_open = false,
  },
  rec = {
    state = TAPE_REC_STATE_EMPTY,
    file = nil,
    pos_tick = 0
  },
  diskfree = 0
}

-- metro for tape
local tape_play_counter = metro[33]
local tape_rec_counter = metro[34]

local DISK_RESERVE = 250
local function tape_diskfree()
  if norns.disk then
    m.diskfree = math.floor((norns.disk - DISK_RESERVE) / .192) -- seconds of 48k/16bit stereo disk free with reserve
  end
end

local function tape_exists(index)
  if type(index) == "number" then
    index = string.format("%04d", index)
  end
  local filename = _path.tape .. index .. ".wav"
  return util.file_exists(filename)
end

local function read_tape_index()
  os.execute("mkdir -p " .. _path.tape)
  local tape = util.os_capture("ls " .. _path.tape, true)
  local t = {}
  for f in tape:gmatch("([^\n]+)") do
    local fs = string.sub(f, 1, 4)
    if tonumber(fs) then
      table.insert(t, tonumber(fs))
    end
  end

  if #t == 0 then
    m.fileindex = 0
  else
    local high = math.max(table.unpack(t))
    m.fileindex = high + 1
  end
end

--- draw a small loop icon aligned to a right-side x position
-- @param x_right number rightmost x where icon should end
-- @param y_center number vertical center of icon
-- @note size tuned for status row in tape UI
local function draw_loop_icon_right(x_right, y_center)
  local w, h, ah   = 11, 7, 4
  local xl, xr     = x_right - w, x_right
  local yt, yb     = y_center - math.ceil(h / 2), y_center - math.ceil(h / 2) + h

  local top_y      = yt + 2
  local bottom_y   = yb - 1
  local left_x     = xl + 1
  local right_x    = xr
  local top_span_x = xr - ah + 1
  local bot_span_x = xl + ah - 1
  local head_r_x   = xr - ah
  local head_l_x   = xl + ah + 1

  screen.aa(0)

  -- frame: top, right, bottom, left
  screen.move(left_x, top_y)
  screen.line(top_span_x, top_y)
  screen.move(right_x, top_y)
  screen.line(right_x, yb - 2)
  screen.move(xr - 1, bottom_y)
  screen.line(bot_span_x, bottom_y)
  screen.move(left_x, yb - 2)
  screen.line(left_x, top_y)
  screen.stroke()

  -- arrowheads: short verticals at ends
  screen.move(head_r_x, yt)
  screen.line(head_r_x, yt + 3)
  screen.move(head_l_x, yb - 3)
  screen.line(head_l_x, yb)
  screen.stroke()

  -- arrow tail pixels
  screen.pixel(xr - 2, yt + 1)
  screen.fill()
  screen.pixel(xl + 1, yb - 2)
  screen.fill()

  screen.aa(1)
end

local function edit_filename(txt)
  if txt then
    m.rec.file = txt .. ".wav"
  else
    _menu.redraw()
    return
  end
  audio.tape_record_open(_path.tape .. m.rec.file)
  m.rec.state = TAPE_REC_STATE_READY
  m.rec.pos_tick = 0
  tape_rec_counter.time = 0.25
  tape_rec_counter.event = function()
    if m.rec.state == TAPE_REC_STATE_RECORDING then
      m.rec.pos_tick = m.rec.pos_tick + 0.25
      if m.rec.pos_tick > m.diskfree then
        print("out of space!")
        audio.tape_record_stop()
        tape_rec_counter:stop()
        -- disarm on out-of-space
        m.rec.state = TAPE_REC_STATE_EMPTY
      end
    end
    if _menu.mode == true and _menu.page == "TAPE" then
      _menu.redraw()
    end
  end
  _menu.redraw()
end


m.key = function(n, z)
  if z ~= 1 then return end -- handle on press only

  if m.mode == TAPE_MODE_PLAY then
    -- helper: open file selector for PLAY
    local function open_play_fileselect()
      local playfile_callback = function(path)
        if path ~= "cancel" then
          audio.tape_play_open(path)
          m.play.file = path:match("[^/]*$")
          m.play.dir_prev = path:match("(.*/)")
          local _, samples, rate = _norns.sound_file_inspect(path)
          m.play.length = math.floor(samples / rate)
          m.play.length_text = util.s_to_hms(m.play.length)
          m.play.pos_tick = 0
          m.play.state = TAPE_PLAY_STATE_READY
          m.play.needs_open = false
          tape_play_counter.time = 0.25
          tape_play_counter.event = function()
            if m.play.state == TAPE_PLAY_STATE_PLAYING then
              m.play.pos_tick = m.play.pos_tick + 0.25
              if m.play.pos_tick > m.play.length then
                if m.play.loop then
                  -- loop to start (displayed time will drift slightly)
                  m.play.pos_tick = m.play.pos_tick - m.play.length
                else
                  -- end reached with loop disabled: stop and mark to reopen on next PLAY
                  audio.tape_play_stop()
                  tape_play_counter:stop()
                  m.play.pos_tick = 0
                  m.play.state = TAPE_PLAY_STATE_READY
                  m.play.needs_open = true
                end
              end
            end
            if _menu.mode == true and _menu.page == "TAPE" then
              _menu.redraw()
            end
          end
        else
          m.play.file = nil
          m.play.state = TAPE_PLAY_STATE_EMPTY
        end
        _menu.redraw()
      end
      fileselect.enter(_path.audio, playfile_callback)
      if m.play.dir_prev ~= nil then
        fileselect.pushd(m.play.dir_prev)
      end
      _menu.redraw()
    end
    -- PLAY mode: K2 = load, K3 = play/pause/resume, both keys open file when EMPTY
    if n == 2 then
      if m.play.state == TAPE_PLAY_STATE_EMPTY then
        open_play_fileselect()
        return
      end
      -- stop playback if active, then load file
      if m.play.state == TAPE_PLAY_STATE_PLAYING or m.play.state == TAPE_PLAY_STATE_PAUSED then
        audio.tape_play_stop()
        tape_play_counter:stop()
        m.play.pos_tick = 0
        m.play.state = TAPE_PLAY_STATE_EMPTY
      end
      open_play_fileselect()
    elseif n == 3 then
      if m.play.state == TAPE_PLAY_STATE_EMPTY then
        open_play_fileselect()
        return
      end
      if m.play.state == TAPE_PLAY_STATE_READY then
        if m.play.needs_open and m.play.dir_prev and m.play.file then
          audio.tape_play_open(m.play.dir_prev .. m.play.file)
          m.play.needs_open = false
        end
        tape_play_counter:start()
        audio.tape_play_start()
        m.play.state = TAPE_PLAY_STATE_PLAYING
      elseif m.play.state == TAPE_PLAY_STATE_PLAYING then
        audio.tape_play_pause(true)
        m.play.state = TAPE_PLAY_STATE_PAUSED
      elseif m.play.state == TAPE_PLAY_STATE_PAUSED then
        audio.tape_play_pause(false)
        m.play.state = TAPE_PLAY_STATE_PLAYING
      end
      _menu.redraw()
    end
  else
    -- REC mode: K2 = arm (with unload confirm), K3 = start/pause/resume. both keys arm when EMPTY.
    local function arm_record_prompt()
      -- prompt for filename (arm) if not actively recording
      if m.rec.state ~= TAPE_REC_STATE_RECORDING then
        tape_diskfree()
        read_tape_index()
        textentry.enter(
          edit_filename,
          string.format("%04d", m.fileindex),
          "tape filename:",
          function(txt)
            if tape_exists(txt) then
              return "FILE EXISTS"
            end
          end
        )
      end
      _menu.redraw()
    end

    local function confirm_unload_then_arm()
      local options = { "cancel", "unload & re-arm" }
      listselect.enter(options, function(choice)
        if choice == "unload & re-arm" then
          -- stop current recording session/file and reset state
          tape_rec_counter:stop()
          audio.tape_record_stop()
          m.rec.state = TAPE_REC_STATE_EMPTY
          m.rec.pos_tick = 0
          m.rec.file = nil
          -- now prompt for new filename
          arm_record_prompt()
        end
        -- cancel: no-op
      end)
    end

    if n == 2 then
      if m.rec.state == TAPE_REC_STATE_EMPTY then
        arm_record_prompt()
      else
        confirm_unload_then_arm()
      end
    elseif n == 3 then
      if m.rec.state == TAPE_REC_STATE_EMPTY then
        arm_record_prompt()
        return
      end
      if m.rec.state == TAPE_REC_STATE_READY then
        tape_rec_counter:start()
        audio.tape_record_start()
        m.rec.state = TAPE_REC_STATE_RECORDING
      elseif m.rec.state == TAPE_REC_STATE_RECORDING then
        audio.tape_record_pause(true)
        m.rec.state = TAPE_REC_STATE_PAUSED
      elseif m.rec.state == TAPE_REC_STATE_PAUSED then
        audio.tape_record_pause(false)
        m.rec.state = TAPE_REC_STATE_RECORDING
      end
      _menu.redraw()
    end
  end
end

m.enc = function(n, d)
  -- E2: switch viewed mode only (no transport change)
  if n == 2 then
    if d > 0 then
      m.mode = TAPE_MODE_REC
    elseif d < 0 then
      m.mode = TAPE_MODE_PLAY
    end
    _menu.redraw()
    -- E3: loop toggle in PLAY mode only
  elseif n == 3 and m.mode == TAPE_MODE_PLAY then
    if d > 0 then
      m.play.loop = true
    elseif d < 0 then
      m.play.loop = false
    end
    audio.tape_play_loop(m.play.loop)
    _menu.redraw()
  end
end

m.gamepad_axis = function(_sensor_axis, _value)
  if gamepad.down() then
    m.mode = TAPE_MODE_REC
    _menu.redraw()
  elseif gamepad.up() then
    m.mode = TAPE_MODE_PLAY
    _menu.redraw()
  end
end

m.redraw = function()
  screen.clear()

  _menu.draw_panel()

  screen.move(128, 10)
  screen.level(m.mode == TAPE_MODE_PLAY and 15 or 1)
  screen.text_right("PLAY")
  screen.level(2)
  screen.rect(0.5, 13.5, 127, 2)
  screen.stroke()

  if m.play.file then
    screen.level(2)
    screen.move(0, 10)
    screen.text(m.play.file)
    screen.move(0, 24)
    screen.text(util.s_to_hms(math.floor(m.play.pos_tick)))
    screen.move(128, 24)
    screen.text_right(m.play.length_text)
    screen.level(15)
    screen.move((m.play.pos_tick / m.play.length * 128), 13.5)
    screen.line_rel(0, 2)
    screen.stroke()
    if m.mode == TAPE_MODE_PLAY then
      local status
      if m.play.state == TAPE_PLAY_STATE_READY then
        status = "PLAY"
      elseif m.play.state == TAPE_PLAY_STATE_PLAYING then
        status = "PAUSE"
      elseif m.play.state == TAPE_PLAY_STATE_PAUSED then
        status = "RESUME"
      else
        status = ""
      end
      -- draw status centered in a fixed column
      screen.level(15)
      screen.move(35, 24)
      screen.text(status)
    end
    local PLAY_LOOP_ICON_RIGHT_X = 95  -- rightmost x position for loop icon
    local PLAY_LOOP_ICON_CENTER_Y = 22 -- vertical center position for loop icon
    if m.mode == TAPE_MODE_PLAY then
      -- play loop: bright when enabled, dim when disabled
      screen.level(m.play.loop and 11 or 1)
      draw_loop_icon_right(PLAY_LOOP_ICON_RIGHT_X, PLAY_LOOP_ICON_CENTER_Y)
    else -- REC focus
      -- play loop: show dim only when enabled; hide when disabled
      if m.play.loop then
        screen.level(1)
        draw_loop_icon_right(PLAY_LOOP_ICON_RIGHT_X, PLAY_LOOP_ICON_CENTER_Y)
      end
    end
  end

  screen.move(128, 48)
  screen.level(m.mode == TAPE_MODE_REC and 15 or 1)
  screen.text_right("REC")
  screen.level(2)
  screen.rect(0.5, 51.5, 127, 2)
  screen.stroke()
  if m.mode == TAPE_MODE_REC then
    screen.level(15)
    screen.move(64, 62)
    if m.rec.state == TAPE_REC_STATE_READY then
      screen.text_center("START")
    elseif m.rec.state == TAPE_REC_STATE_RECORDING then
      screen.text_center("PAUSE")
    elseif m.rec.state == TAPE_REC_STATE_PAUSED then
      screen.text_center("RESUME")
    end
  end
  if m.rec.state ~= TAPE_REC_STATE_EMPTY then
    screen.level(1)
    screen.move(0, 48)
    screen.text(m.rec.file)
    screen.level(2)
    screen.move(0, 62)
    screen.text(util.s_to_hms(math.floor(m.rec.pos_tick)))
  end
  screen.level(2)
  screen.move(127, 62)
  screen.text_right(util.s_to_hms(m.diskfree))
  screen.level(15)
  screen.move((m.rec.pos_tick / m.diskfree * 128), 51.5)
  screen.line_rel(0, 2)
  screen.stroke()

  screen.update()
end

m.init = function()
  tape_diskfree()
end
m.deinit = norns.none

return m
