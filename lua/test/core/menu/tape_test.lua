-- lua/test/core/menu/tape_test.lua
-- unit tests for tape menu module

local luaunit = require('lib/test/luaunit')
local mock = require('lib/test/mock')

-- helper to set up a fresh test environment and load the module
local function setup_tape_env(opts)
  opts = opts or {}

  -- stub globals
  _path = { audio = "/audio/", tape = "/tape/" }

  -- engine stubs
  _norns = {
    tape_status = function(_) end,
    tape_play_file = function(_) end,
    tape_rec_file = function(_) end,
    tape_play_stop = mock.spy(),
    tape_play_start = mock.spy(),
    tape_play_pause = mock.spy(),
    tape_play_open = mock.spy(),
    tape_play_loop = mock.spy(),
    tape_record_start = mock.spy(),
    tape_record_pause = mock.spy(),
    tape_record_stop = mock.spy(),
    tape_record_open = mock.spy(),
  }

  -- menu spies
  _menu = { redraw = mock.spy(), draw_panel = mock.spy() }

  -- minimal screen stubs (only spy on update, others are no-ops)
  screen = {
    aa = function() end,
    move = function() end,
    line = function() end,
    line_rel = function() end,
    stroke = function() end,
    pixel = function() end,
    fill = function() end,
    level = function() end,
    rect = function() end,
    text = function() end,
    text_right = function() end,
    text_center = function() end,
    clear = function() end,
    update = mock.spy(),
  }

  -- stub controlspec
  package.loaded['controlspec'] = { new = function() return {} end }
  -- real audio module for state/getters
  package.loaded['core/audio'] = nil
  audio = require('core/audio')

  -- gamepad stubs
  gamepad = {
    up = function() return false end,
    down = function() return false end,
  }

  -- norns environment
  norns = {
    none = function() end,
    disk = opts.disk or 1000,
  }

  -- util stub module
  local util_stub = {}
  util_stub._os_capture_output = opts.os_capture_output or ""
  util_stub._existing_files = {}
  function util_stub.os_capture(_, _)
    return util_stub._os_capture_output or ""
  end

  function util_stub.file_exists(path)
    local map = util_stub._existing_files or {}
    return map[path] == true
  end

  function util_stub.s_to_hms(_)
    return "0:00"
  end

  function util_stub.clamp(x, min, max)
    if x < min then return min end
    if x > max then return max end
    return x
  end

  -- fileselect stub module
  local fileselect_stub = {}
  fileselect_stub.enter = function(path, cb)
    fileselect_stub._last_path = path
    fileselect_stub._last_callback = cb
  end
  fileselect_stub.pushd = mock.spy()

  -- textentry stub module
  local textentry_stub = {}
  textentry_stub.enter = function(cb, initial, label, validator)
    textentry_stub._last_callback = cb
    textentry_stub._last_initial = initial
    textentry_stub._last_label = label
    textentry_stub._last_validator = validator
  end

  -- listselect stub module
  local listselect_stub = {}
  listselect_stub.enter = function(options, cb)
    listselect_stub._last_options = options
    listselect_stub._last_callback = cb
  end

  -- stub required modules before requiring tape
  package.loaded['util'] = util_stub
  package.loaded['fileselect'] = fileselect_stub
  package.loaded['textentry'] = textentry_stub
  package.loaded['listselect'] = listselect_stub

  -- stub os.execute invoked by read_tape_index
  local original_os_execute = os.execute
  os.execute = function(_) return true end -- luacheck: ignore

  -- ensure fresh load
  package.loaded['core/menu/tape'] = nil
  local tape = require('core/menu/tape')

  -- simulate startup wiring so menu references work during tests
  _norns.tape_status = function(ps, pp, pl, rs, rp, loop) audio.tape._on_status(ps, pp, pl, rs, rp, loop) end
  _norns.tape_play_file = function(path) audio.tape._on_play_file(path) end
  _norns.tape_rec_file = function(path) audio.tape._on_rec_file(path) end

  -- by default, initialize the tape menu to subscribe to state updates
  if tape and tape.init then tape.init() end

  -- convenience: expose restore to clean up package.loaded and os
  local function restore()
    -- ensure we unsubscribe to avoid cross-test observers
    if tape and tape.deinit then pcall(tape.deinit) end
    package.loaded['core/menu/tape'] = nil
    package.loaded['util'] = nil
    package.loaded['fileselect'] = nil
    package.loaded['textentry'] = nil
    package.loaded['listselect'] = nil
    os.execute = original_os_execute -- luacheck: ignore
  end

  return {
    tape = tape,
    util = util_stub,
    fileselect = fileselect_stub,
    textentry = textentry_stub,
    listselect = listselect_stub,
    restore = restore,
  }
end

TestTape = {}

-- helpers to drive centralized tape state (engine-as-truth)
local function set_play_state(state)
  local snap = audio.tape.get_state()
  _norns.tape_status(state, 0, 0, snap.rec.state, 0, (snap.play.loop and 1 or 0))
end
local function set_rec_state(state)
  local snap = audio.tape.get_state()
  _norns.tape_status(snap.play.state, 0, 0, state, 0, (snap.play.loop and 1 or 0))
end

function TestTape.test_enc_mode_switch_changes_mode()
  -- E2 toggles between PLAY and REC modes
  local env = setup_tape_env()
  local tape = env.tape
  local TM = tape.constants
  luaunit.assertEquals(tape.mode, TM.TAPE_MODE_PLAY)

  tape.enc(2, 1)

  luaunit.assertEquals(tape.mode, TM.TAPE_MODE_REC)

  tape.enc(2, -1)

  luaunit.assertEquals(tape.mode, TM.TAPE_MODE_PLAY)

  env.restore()
end

function TestTape.test_enc_loop_toggle_in_play_mode()
  -- E3 toggles loop state when in PLAY mode
  local env = setup_tape_env()
  local tape = env.tape
  local TM = tape.constants
  luaunit.assertEquals(tape.mode, TM.TAPE_MODE_PLAY)

  _norns.tape_play_loop.reset()
  tape.enc(3, 1)

  luaunit.assertTrue(_norns.tape_play_loop.called())
  luaunit.assertEquals(_norns.tape_play_loop.args(1)[1], 1)

  _norns.tape_play_loop.reset()
  tape.enc(3, -1)

  luaunit.assertTrue(_norns.tape_play_loop.called())
  luaunit.assertEquals(_norns.tape_play_loop.args(1)[1], 0)

  env.restore()
end

function TestTape.test_enc_loop_noop_in_rec_mode()
  -- E3 loop toggle is ignored when in REC mode
  local env = setup_tape_env()
  local tape = env.tape
  local TM = tape.constants
  tape.mode = TM.TAPE_MODE_REC

  _norns.tape_play_loop.reset()
  tape.enc(3, 1)

  luaunit.assertFalse(_norns.tape_play_loop.called())

  env.restore()
end

function TestTape.test_key_play_k2_opens_fileselect_when_empty()
  -- K2 in PLAY opens file select when no file is loaded
  local env = setup_tape_env()
  local tape = env.tape
  local C = audio.tape.constants
  set_play_state(C.TAPE_PLAY_STATE_EMPTY)
  env.fileselect._last_callback = nil
  env.fileselect._last_path = nil

  tape.key(2, 1)

  luaunit.assertEquals(env.fileselect._last_path, _path.audio)

  env.restore()
end

function TestTape.test_key_play_k2_stops_when_active_then_opens()
  -- K2 in PLAY stops active playback (or paused) and opens file select
  local env = setup_tape_env()
  local tape = env.tape
  local C = audio.tape.constants

  set_play_state(C.TAPE_PLAY_STATE_PLAYING)
  _norns.tape_play_stop.reset()
  env.fileselect._last_path = nil
  tape.key(2, 1)

  luaunit.assertTrue(_norns.tape_play_stop.called())
  luaunit.assertEquals(env.fileselect._last_path, _path.audio)

  set_play_state(C.TAPE_PLAY_STATE_PAUSED)
  _norns.tape_play_stop.reset()
  env.fileselect._last_path = nil
  tape.key(2, 1)

  luaunit.assertTrue(_norns.tape_play_stop.called())
  luaunit.assertEquals(env.fileselect._last_path, _path.audio)

  env.restore()
end

function TestTape.test_no_redundant_redraws_from_handlers_only_state_updates()
  -- key/enc/gamepad handlers should not call _menu.redraw directly; redraws come from state updates
  local env = setup_tape_env()
  local tape = env.tape
  local C = audio.tape.constants

  -- start from READY with a file loaded
  _norns.tape_status(C.TAPE_PLAY_STATE_READY, 0.0, 10.0, C.TAPE_REC_STATE_READY, 0.0, 1)

  -- enc mode switch should not redraw
  _menu.redraw.reset()
  tape.enc(2, 1) -- to REC
  tape.enc(2, -1) -- back to PLAY
  luaunit.assertFalse(_menu.redraw.called())

  -- loop toggle should not redraw
  _menu.redraw.reset()
  tape.enc(3, 1)
  luaunit.assertFalse(_menu.redraw.called())

  -- play/pause via key should not redraw; but the following status update should
  _menu.redraw.reset()
  _norns.tape_play_start.reset()
  tape.key(3, 1) -- from READY -> start
  luaunit.assertTrue(_norns.tape_play_start.called())
  luaunit.assertFalse(_menu.redraw.called())
  -- now simulate engine status update triggers redraw
  _norns.tape_status(C.TAPE_PLAY_STATE_PLAYING, 1.0, 10.0, C.TAPE_REC_STATE_READY, 0.0, 1)
  luaunit.assertTrue(_menu.redraw.called())

  env.restore()
end

function TestTape.test_key_play_k3_behavior_by_state()
  -- K3 in PLAY mode: opens file select when empty, otherwise controls playback
  local env = setup_tape_env()
  local tape = env.tape
  local C = audio.tape.constants

  set_play_state(C.TAPE_PLAY_STATE_EMPTY)
  env.fileselect._last_path = nil
  tape.key(3, 1)

  luaunit.assertEquals(env.fileselect._last_path, _path.audio)

  set_play_state(C.TAPE_PLAY_STATE_READY)
  _norns.tape_play_start.reset()
  tape.key(3, 1)

  luaunit.assertTrue(_norns.tape_play_start.called())

  set_play_state(C.TAPE_PLAY_STATE_PLAYING)
  _norns.tape_play_pause.reset()
  tape.key(3, 1)

  luaunit.assertTrue(_norns.tape_play_pause.called())
  luaunit.assertEquals(_norns.tape_play_pause.args(1)[1], 1)

  set_play_state(C.TAPE_PLAY_STATE_PAUSED)
  _norns.tape_play_pause.reset()
  tape.key(3, 1)

  luaunit.assertTrue(_norns.tape_play_pause.called())
  luaunit.assertEquals(_norns.tape_play_pause.args(1)[1], 0)

  env.restore()
end

function TestTape.test_fileselect_callback_sets_dir_and_opens_file()
  -- file select callback opens the chosen file and stores its directory
  local env = setup_tape_env()
  local tape = env.tape
  local C = audio.tape.constants

  set_play_state(C.TAPE_PLAY_STATE_EMPTY)
  tape.key(2, 1) -- trigger file select to capture callback
  _norns.tape_play_open.reset()
  local chosen = "/music/some/file.wav"
  env.fileselect._last_callback(chosen)

  luaunit.assertTrue(_norns.tape_play_open.called())
  luaunit.assertEquals(_norns.tape_play_open.args(1)[1], chosen)
  luaunit.assertEquals(tape.dir_prev, "/music/some/")

  _norns.tape_play_open.reset()
  env.fileselect._last_callback("cancel")

  luaunit.assertFalse(_norns.tape_play_open.called())

  env.restore()
end

function TestTape.test_fileselect_pushd_uses_previous_dir()
  -- file select pushd navigates to the previously used directory
  local env = setup_tape_env()
  local tape = env.tape
  local C = audio.tape.constants
  tape.dir_prev = "/prev/dir/"
  set_play_state(C.TAPE_PLAY_STATE_EMPTY)
  env.fileselect.pushd.reset()

  tape.key(2, 1)

  luaunit.assertTrue(env.fileselect.pushd.called())
  luaunit.assertEquals(env.fileselect.pushd.args(1)[1], "/prev/dir/")

  env.restore()
end

function TestTape.test_rec_k2_arms_when_empty_and_validates_filename()
  -- K2 in REC prompts for a filename and rejects duplicates
  local env = setup_tape_env({ os_capture_output = "" })
  local tape = env.tape
  local TM = tape.constants
  local C = audio.tape.constants
  tape.mode = TM.TAPE_MODE_REC
  set_rec_state(C.TAPE_REC_STATE_EMPTY)
  env.textentry._last_callback = nil

  -- open text entry on K2 release (avoid immediate cancel)
  tape.key(2, 0)

  luaunit.assertEquals(env.textentry._last_label, "tape filename:")
  luaunit.assertEquals(env.textentry._last_initial, string.format("%04d", tape.fileindex))

  local validator = env.textentry._last_validator
  local target = "/tape/0005.wav"
  env.util._existing_files[target] = true

  luaunit.assertEquals(validator("0005"), "FILE EXISTS")
  luaunit.assertIsNil(validator("9999"))

  env.restore()
end

function TestTape.test_edit_filename_callback_opens_recording()
  -- filename edit callback arms recording when given valid text
  local env = setup_tape_env()
  local tape = env.tape
  local TM = tape.constants
  local C = audio.tape.constants
  tape.mode = TM.TAPE_MODE_REC
  set_rec_state(C.TAPE_REC_STATE_EMPTY)

  -- open confirm on K2 release
  tape.key(2, 0)
  local edit_cb = env.textentry._last_callback

  _norns.tape_record_open.reset()
  edit_cb(nil)

  luaunit.assertFalse(_norns.tape_record_open.called())

  _norns.tape_record_open.reset()
  edit_cb("take1")

  luaunit.assertTrue(_norns.tape_record_open.called())
  luaunit.assertEquals(_norns.tape_record_open.args(1)[1], _path.tape .. "take1.wav")

  env.restore()
end

function TestTape.test_key_rec_k3_behavior_by_state()
  -- K3 in REC mode: opens filename prompt when empty, otherwise controls recording
  local env = setup_tape_env()
  local tape = env.tape
  local TM = tape.constants
  local C = audio.tape.constants
  tape.mode = TM.TAPE_MODE_REC

  set_rec_state(C.TAPE_REC_STATE_EMPTY)
  env.textentry._last_callback = nil
  tape.key(3, 1)

  luaunit.assertNotIsNil(env.textentry._last_callback)

  set_rec_state(C.TAPE_REC_STATE_READY)
  _norns.tape_record_start.reset()
  tape.key(3, 1)

  luaunit.assertTrue(_norns.tape_record_start.called())

  set_rec_state(C.TAPE_REC_STATE_RECORDING)
  _norns.tape_record_pause.reset()
  tape.key(3, 1)

  luaunit.assertTrue(_norns.tape_record_pause.called())
  luaunit.assertEquals(_norns.tape_record_pause.args(1)[1], 1)

  set_rec_state(C.TAPE_REC_STATE_PAUSED)
  _norns.tape_record_pause.reset()
  tape.key(3, 1)

  luaunit.assertTrue(_norns.tape_record_pause.called())
  luaunit.assertEquals(_norns.tape_record_pause.args(1)[1], 0)

  env.restore()
end

function TestTape.test_key_rec_k2_confirms_unload_then_reset()
  -- K2 in REC shows confirm dialog to unload (reset)
  local env = setup_tape_env()
  local tape = env.tape
  local TM = tape.constants
  local C = audio.tape.constants
  tape.mode = TM.TAPE_MODE_REC
  set_rec_state(C.TAPE_REC_STATE_RECORDING)

  -- open confirm on K2 release
  tape.key(2, 0)

  luaunit.assertNotIsNil(env.listselect._last_options)
  luaunit.assertNotIsNil(env.listselect._last_callback)

  _norns.tape_record_stop.reset()
  env.textentry._last_callback = nil
  -- simulate engine state change after stopping before re-arming
  set_rec_state(C.TAPE_REC_STATE_READY)
  env.listselect._last_callback("unload")

  luaunit.assertTrue(_norns.tape_record_stop.called())
  luaunit.assertIsNil(env.textentry._last_callback)

  _norns.tape_record_stop.reset()
  env.textentry._last_callback = nil
  env.listselect._last_callback("cancel")

  luaunit.assertFalse(_norns.tape_record_stop.called())
  luaunit.assertIsNil(env.textentry._last_callback)

  env.restore()
end

function TestTape.test_redraw_updates_diskfree_and_calls_update()
  -- redraw draws panel + updates screen; diskfree string reflects formatted compute result
  local env = setup_tape_env({ disk = 1000 })
  local tape = env.tape
  local C = audio.tape.constants

  -- stub core compute + formatter to avoid duplicating math in menu tests
  audio.tape_compute_diskfree = function() return 4242 end
  env.util.s_to_hms = function(_) return "DF_STUB" end

  -- trigger a state update so the menu recalculates diskfree_str using stubs
  _norns.tape_status(C.TAPE_PLAY_STATE_READY, 0, 0, C.TAPE_REC_STATE_READY, 0, 1)

  screen.update.reset()
  _menu.draw_panel.reset()

  -- redraw should invoke draw panel + screen update; diskfree_str already updated by subscription
  tape.redraw()

  luaunit.assertEquals(tape.diskfree_str, "DF_STUB")
  luaunit.assertTrue(_menu.draw_panel.called())
  luaunit.assertTrue(screen.update.called())

  env.restore()
end

function TestTape.test_init_sets_diskfree()
  -- init computes and sets diskfree string using core compute and formatter
  local env = setup_tape_env({ disk = 500 })
  local tape = env.tape

  -- ensure fresh init after stubbing
  if tape.deinit then tape.deinit() end

  -- stub core compute + formatter to avoid duplicating math in menu tests
  audio.tape_compute_diskfree = function() return 3600 end
  env.util.s_to_hms = function(_) return "INIT_DF_STUB" end

  tape.init()

  luaunit.assertEquals(tape.diskfree_str, "INIT_DF_STUB")

  env.restore()
end

function TestTape.test_init_registers_and_deinit_unregisters_observer()
  -- init subscribes to tape state and triggers redraw on updates; deinit unsubscribes
  local env = setup_tape_env({ disk = 500 })
  local tape = env.tape
  local C = audio.tape.constants

  _menu.redraw.reset()
  tape.init()

  -- simulate an incoming tape status update; expect menu redraw
  _norns.tape_status(C.TAPE_PLAY_STATE_READY, 1.0, 120.0, C.TAPE_REC_STATE_READY, 0.0, 1)
  luaunit.assertTrue(_menu.redraw.called())

  -- deinit should unsubscribe; further status updates should not cause menu redraws
  _menu.redraw.reset()
  tape.deinit()
  _norns.tape_status(C.TAPE_PLAY_STATE_PLAYING, 2.0, 120.0, C.TAPE_REC_STATE_READY, 0.0, 1)
  luaunit.assertFalse(_menu.redraw.called())

  env.restore()
end

function TestTape.test_key_ignores_release_events()
  -- key handler ignores release events (z == 0)
  local env = setup_tape_env()
  local tape = env.tape
  env.fileselect._last_path = nil

  tape.key(2, 0) -- release

  luaunit.assertIsNil(env.fileselect._last_path)

  env.restore()
end

function TestTape.test_gamepad_axis_switches_modes()
  -- gamepad down switches to REC; up switches to PLAY
  local env = setup_tape_env()
  local tape = env.tape
  local TM = tape.constants

  function gamepad.down() return true end

  function gamepad.up() return false end

  _menu.redraw.reset()
  tape.gamepad_axis(nil, nil)

  luaunit.assertEquals(tape.mode, TM.TAPE_MODE_REC)

  function gamepad.down() return false end

  function gamepad.up() return true end

  _menu.redraw.reset()
  tape.gamepad_axis(nil, nil)

  luaunit.assertEquals(tape.mode, TM.TAPE_MODE_PLAY)

  env.restore()
end

function TestTape.test_loop_highlight_reflects_audio_loop_flag()
  -- menu loop highlight tracks audio.tape loop state from engine status
  local env = setup_tape_env()
  local tape = env.tape
  local C = audio.tape.constants

  -- ensure we have a known state; toggle loop on
  _norns.tape_status(C.TAPE_PLAY_STATE_READY, 0.0, 0.0, C.TAPE_REC_STATE_READY, 0.0, 1)
  luaunit.assertTrue(tape.play.loop_enabled)

  -- toggle loop off from engine
  _norns.tape_status(C.TAPE_PLAY_STATE_READY, 0.0, 0.0, C.TAPE_REC_STATE_READY, 0.0, 0)
  luaunit.assertFalse(tape.play.loop_enabled)

  env.restore()
end
