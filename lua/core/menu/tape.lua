local fileselect = require 'fileselect'
local textentry = require 'textentry'
local listselect = require 'listselect'
local util = require 'util'

-- ensure audio.tape has been initialized
assert(audio and audio.tape and audio.tape.constants,
  "tape constants not available -- audio.tape should be initialized before menu.tape")

local C = audio.tape.constants
local TAPE_MODE_PLAY = 1 -- play section is focused
local TAPE_MODE_REC = 2  -- record section is focused

local m = {
  mode         = TAPE_MODE_PLAY, -- current focused section
  dir_prev     = nil,            -- last directory used in fileselect
  play         = {
    state = C.TAPE_PLAY_STATE_EMPTY,
    file = nil,
    pos = 0,
    len = 0,
    loop_enabled = false,
    status_label = "",
    pos_str = "0:00:00",
    len_str = "0:00:00",
    progress = 0 -- position for progress bar
  },
  rec          = {
    state = C.TAPE_REC_STATE_EMPTY,
    file = nil,
    pos = 0,
    status_label = "",
    pos_str = "0:00:00",
    len_str = "0:00:00",            -- available record time derived from disk free
    should_display_progress = true, -- true when diskfree > 0
    progress = 0                    -- position for progress bar
  },
  diskfree     = 0,
  diskfree_str = "0:00:00"
}

-- expose only UI-specific constants; tape state constants live in audio.tape.constants
m.constants = {
  TAPE_MODE_PLAY = TAPE_MODE_PLAY,
  TAPE_MODE_REC = TAPE_MODE_REC,
}

-- derive menu-facing state from centralized tape snapshot
local function update_state(current_state)
  if not current_state then return end

  local diskfree = audio.tape_compute_diskfree()
  m.diskfree_str = util.s_to_hms(diskfree)

  -- PLAY
  local p = current_state.play or {}
  m.play.state = p.state
  m.play.file = p.file
  m.play.pos = p.pos or 0
  m.play.len = p.len or 0
  m.play.loop_enabled = p.loop == true
  m.play.pos_str = util.s_to_hms(math.floor(m.play.pos))
  m.play.len_str = util.s_to_hms(math.floor(m.play.len))
  if m.play.len > 0 then
    m.play.progress = (m.play.pos / m.play.len * 128)
  else
    m.play.progress = 0
  end
  if p.state == C.TAPE_PLAY_STATE_READY then
    m.play.status_label = "PLAY"
  elseif p.state == C.TAPE_PLAY_STATE_PLAYING then
    m.play.status_label = "PAUSE"
  elseif p.state == C.TAPE_PLAY_STATE_PAUSED then
    m.play.status_label = "RESUME"
  else
    m.play.status_label = ""
  end

  -- REC
  local r = current_state.rec or {}
  m.rec.state = r.state
  m.rec.file = r.file
  m.rec.pos = r.pos or 0
  m.rec.pos_str = util.s_to_hms(math.floor(m.rec.pos))
  m.rec.should_display_progress = diskfree > 0
  m.rec.progress = util.clamp((m.rec.pos / diskfree) * 128, 0, 127)
  if r.state == C.TAPE_REC_STATE_READY then
    m.rec.status_label = "START"
  elseif r.state == C.TAPE_REC_STATE_RECORDING then
    m.rec.status_label = "PAUSE"
  elseif r.state == C.TAPE_REC_STATE_PAUSED then
    m.rec.status_label = "RESUME"
  else
    m.rec.status_label = ""
  end

end

-- file system helpers
local function file_tape_exists(index)
  if type(index) == "number" then
    index = string.format("%04d", index)
  end
  local filename = _path.tape .. index .. ".wav"
  return util.file_exists(filename)
end

local function file_read_tape_index()
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

-- transport controls
local function stop_playback() audio.tape_play_stop() end
local function start_playback() audio.tape_play_start() end
local function pause_playback() audio.tape_play_pause(true) end
local function resume_playback() audio.tape_play_pause(false) end
local function start_recording() audio.tape_record_start() end
local function pause_recording() audio.tape_record_pause(true) end
local function resume_recording() audio.tape_record_pause(false) end
local function stop_recording() audio.tape_record_stop() end

-- recording setup
local function arm_recording(filename)
  if not filename or filename == "" then return end
  audio.tape_record_open(_path.tape .. filename .. ".wav")
end

local function edit_filename(txt)
  if not txt then
    return
  end

  arm_recording(txt)
end

--- ui
--
--- draw a small loop icon aligned to a right-side x position
-- @param x_right number rightmost x where icon should end
-- @param y_center number vertical center of icon
-- @note size tuned for status row in tape ui
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

-- draw play header
local function draw_play_header()
  screen.move(128, 10)
  screen.level(m.mode == TAPE_MODE_PLAY and 15 or 1)
  screen.text_right("PLAY")
  screen.level(2)
  screen.rect(0.5, 13.5, 127, 2)
  screen.stroke()
end

-- file dialog
local function open_play_fileselect()
  local playfile_callback = function(path)
    if path ~= "cancel" then
      audio.tape_play_open(path)
      m.dir_prev = path:match("(.*/)")
    end
  end
  fileselect.enter(_path.audio, playfile_callback)
  if m.dir_prev ~= nil then
    fileselect.pushd(m.dir_prev)
  end
end

-- prompt for recording filename
local function arm_record_prompt()
  -- only display if not actively recording
  local should_display_prompt = (m.rec.state == C.TAPE_REC_STATE_EMPTY) or
      (m.rec.state == C.TAPE_REC_STATE_READY) or
      (m.rec.state == C.TAPE_REC_STATE_PAUSED)
  if not should_display_prompt then return end
  file_read_tape_index()
  textentry.enter(
    edit_filename,
    string.format("%04d", m.fileindex),
    "tape filename:",
    function(txt)
      if file_tape_exists(txt) then
        return "FILE EXISTS"
      end
    end
  )
end

-- confirm unload current recording and reset REC state (no automatic re-arm)
local function confirm_unload_then_reset()
  local options = { "cancel", "unload" }
  listselect.enter(options, function(choice)
    if choice == "unload" then
      -- stop current recording session/file and reset REC state
      stop_recording()
    end
    -- cancel: no-op
  end)
end

local function draw_play_file_info()
  if not m.play.file then return end

  screen.level(2)
  screen.move(0, 10)
  screen.text(m.play.file)

  -- position and length
  screen.move(0, 24)
  screen.text(m.play.pos_str)
  screen.move(128, 24)
  screen.text_right(m.play.len_str)

  -- progress bar
  screen.level(15)
  screen.move(m.play.progress, 13.5)
  screen.line_rel(0, 2)
  screen.stroke()
end

local function draw_play_status()
  if not m.play.file or m.mode ~= TAPE_MODE_PLAY then return end

  -- draw status centered in a fixed column
  screen.level(15)
  screen.move(35, 24)
  screen.text(m.play.status_label)
end

local function draw_play_loop_icon()
  if not m.play.file then return end

  local loop_enabled = m.play.loop_enabled
  if m.mode == TAPE_MODE_PLAY then
    -- loop is bright when enabled; dim when disabled
    screen.level(loop_enabled and 11 or 1)
    draw_loop_icon_right(95, 22)
  else -- REC focus
    -- loop displays dim only when enabled; hide when disabled
    if loop_enabled then
      screen.level(1)
      draw_loop_icon_right(95, 22)
    end
  end
end

local function draw_rec_header()
  screen.move(128, 48)
  screen.level(m.mode == TAPE_MODE_REC and 15 or 1)
  screen.text_right("REC")
  screen.level(2)
  screen.rect(0.5, 51.5, 127, 2)
  screen.stroke()
end

local function draw_rec_status()
  if m.mode ~= TAPE_MODE_REC then return end

  screen.level(15)
  screen.move(64, 62)
  screen.text_center(m.rec.status_label)
end

local function draw_rec_file_info()
  local rec_state = m.rec.state
  local rec_file = m.rec.file
  if rec_state ~= C.TAPE_REC_STATE_EMPTY and rec_file then
    screen.level(1)
    screen.move(0, 48)
    screen.text(rec_file)
    screen.level(2)
    screen.move(0, 62)
    screen.text(m.rec.pos_str)
  end

  -- disk free and progress bar
  screen.level(2)
  screen.move(127, 62)
  screen.text_right(m.diskfree_str)

  if m.rec.should_display_progress then
    screen.level(15)
    screen.move(m.rec.progress, 51.5)
    screen.line_rel(0, 2)
    screen.stroke()
  end
end

--- key handler helpers
--

-- PLAY mode key handling
local function handle_play_keys(n, z)
  -- K2 = load, K3 = play/pause/resume
  -- when play state is empty, both K2 and K3 trigger load
  if z == 0 then return end -- ignore key releases

  local play_state = m.play.state
  if n == 2 then
    if play_state == C.TAPE_PLAY_STATE_EMPTY then
      open_play_fileselect()
      return
    end
    -- stop playback if active, then load file
    if play_state == C.TAPE_PLAY_STATE_PLAYING or play_state == C.TAPE_PLAY_STATE_PAUSED then
      stop_playback()
    end
    open_play_fileselect()
  elseif n == 3 then
    if play_state == C.TAPE_PLAY_STATE_EMPTY then
      open_play_fileselect()
      return
    end
    if play_state == C.TAPE_PLAY_STATE_READY then
      start_playback()
    elseif play_state == C.TAPE_PLAY_STATE_PLAYING then
      pause_playback()
    elseif play_state == C.TAPE_PLAY_STATE_PAUSED then
      resume_playback()
    end
  end
end

-- REC mode key handling
local function handle_rec_keys(n, z)
  -- K2 = arm/unload (reset), K3 = start/pause/resume
  -- K2 triggers actions on release (z==0) to avoid immediate cancel within textentry
  local rec_state = m.rec.state
  if n == 2 then
    if z == 0 then
      if rec_state == C.TAPE_REC_STATE_EMPTY then
        arm_record_prompt()
      else
        confirm_unload_then_reset()
      end
    end
  elseif n == 3 then
    if z == 1 then
      if rec_state == C.TAPE_REC_STATE_EMPTY then
        arm_record_prompt()
        return
      end
      if rec_state == C.TAPE_REC_STATE_READY then
        start_recording()
      elseif rec_state == C.TAPE_REC_STATE_RECORDING then
        pause_recording()
      elseif rec_state == C.TAPE_REC_STATE_PAUSED then
        resume_recording()
      end
    end
  end
end


m.key = function(n, z)
  local actions = {
    [TAPE_MODE_PLAY] = handle_play_keys,
    [TAPE_MODE_REC] = handle_rec_keys,
  }
  local action = actions[m.mode]
  if action then action(n, z) end
end

m.enc = function(n, d)
  -- E2 = switch tape mode
  -- E3 = loop toggle in play mode only
  if n == 2 then
    if d > 0 then
      m.mode = TAPE_MODE_REC
    elseif d < 0 then
      m.mode = TAPE_MODE_PLAY
    end
  elseif n == 3 and m.mode == TAPE_MODE_PLAY then
    local new_loop = (d > 0)
    audio.tape_play_loop(new_loop)
  end
end

m.gamepad_axis = function(_sensor_axis, _value)
  if gamepad.down() then
    m.mode = TAPE_MODE_REC
  elseif gamepad.up() then
    m.mode = TAPE_MODE_PLAY
  end
end

m.redraw = function()
  screen.clear()
  _menu.draw_panel()

  -- draw PLAY section
  draw_play_header()
  draw_play_file_info()
  draw_play_status()
  draw_play_loop_icon()

  -- draw REC section
  draw_rec_header()
  draw_rec_status()
  draw_rec_file_info()

  screen.update()
end

m.init = function()
  -- subscribe to central tape state; redraw on updates
  assert(audio and audio.tape, "audio.tape not available -- audio.tape should be initialized before menu.tape")
  -- register to receive tape state updates from audio.tape
  audio.tape.subscribe(m, function(current_state)
    update_state(current_state)
    _menu.redraw()
  end)
  -- initialize from current state
  update_state(audio.tape.get_state())
  _menu.redraw()
end

m.deinit = function()
  assert(audio and audio.tape, "audio.tape not available -- audio.tape should be initialized before menu.tape")
  -- unsubscribe from tape state updates
  audio.tape.unsubscribe(m)
end

return m
