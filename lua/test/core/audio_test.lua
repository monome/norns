-- lua/test/core/audio_test.lua
-- unit tests for audio.tape state/control

local luaunit = require('lib/test/luaunit')
local mock = require('lib/test/mock')

local function setup_core_audio_env(opts)
  opts = opts or {}

  -- stub globals used by audio
  norns = {
    disk = opts.disk or 1000,
  }

  -- engine control spies
  _norns = {
    tape_play_open = mock.spy(),
    tape_play_start = mock.spy(),
    tape_play_pause = mock.spy(),
    tape_play_stop = mock.spy(),
    tape_play_loop = mock.spy(),
    tape_record_open = mock.spy(),
    tape_record_start = mock.spy(),
    tape_record_pause = mock.spy(),
    tape_record_stop = mock.spy(),
  }

  -- stub controlspec
  package.loaded['controlspec'] = { new = function() return {} end }

  -- ensure fresh module load
  package.loaded['core/audio'] = nil
  local audio = require('core/audio')

  return {
    audio = audio,
  }
end

TestAudioTape = {}

function TestAudioTape.test_constants_exposed()
  local env = setup_core_audio_env()
  local C = env.audio.tape.constants

  luaunit.assertEquals(type(C), 'table')
  luaunit.assertEquals(C.TAPE_PLAY_STATE_EMPTY, 0)
  luaunit.assertEquals(C.TAPE_PLAY_STATE_READY, 1)
  luaunit.assertEquals(C.TAPE_PLAY_STATE_PLAYING, 2)
  luaunit.assertEquals(C.TAPE_PLAY_STATE_PAUSED, 3)
  luaunit.assertEquals(C.TAPE_REC_STATE_EMPTY, 0)
  luaunit.assertEquals(C.TAPE_REC_STATE_READY, 1)
  luaunit.assertEquals(C.TAPE_REC_STATE_RECORDING, 2)
  luaunit.assertEquals(C.TAPE_REC_STATE_PAUSED, 3)
end

function TestAudioTape.test_initial_snapshot_and_getters()
  local env = setup_core_audio_env()
  local audio = env.audio

  local snap = audio.tape.get_state()
  luaunit.assertEquals(snap.play.state, audio.tape.constants.TAPE_PLAY_STATE_EMPTY)
  luaunit.assertEquals(snap.play.pos, 0)
  luaunit.assertEquals(snap.play.len, 0)
  luaunit.assertEquals(snap.play.loop, true)
  luaunit.assertIsNil(snap.play.file)

  luaunit.assertEquals(snap.rec.state, audio.tape.constants.TAPE_REC_STATE_EMPTY)
  luaunit.assertEquals(snap.rec.pos, 0)
  luaunit.assertIsNil(snap.rec.file)

  luaunit.assertFalse(audio.tape_is_recording())
end

function TestAudioTape.test_on_status_updates_state_and_notifies()
  local env = setup_core_audio_env()
  local audio = env.audio
  local C = audio.tape.constants

  local observed = {}
  local calls = 0
  audio.tape.subscribe(observed, function(value)
    calls = calls + 1
    observed.last = value
  end)

  audio.tape._on_status(C.TAPE_PLAY_STATE_READY, 12.3, 120.0, C.TAPE_REC_STATE_READY, 0.5, 1)

  luaunit.assertEquals(calls, 1)
  local snap = audio.tape.get_state()
  luaunit.assertEquals(snap.play.state, C.TAPE_PLAY_STATE_READY)
  luaunit.assertEquals(snap.rec.state, C.TAPE_REC_STATE_READY)
  luaunit.assertEquals(snap.play.pos, 12.3)
  luaunit.assertEquals(snap.play.len, 120.0)
  luaunit.assertTrue(snap.play.loop)
  luaunit.assertEquals(observed.last.play.pos, 12.3)
  luaunit.assertEquals(observed.last.rec.pos, 0.5)

  -- toggle loop off and playing/recording state
  audio.tape._on_status(C.TAPE_PLAY_STATE_PLAYING, 20, 120, C.TAPE_REC_STATE_RECORDING, 1.25, 0)
  local snap2 = audio.tape.get_state()
  luaunit.assertEquals(snap2.play.state, C.TAPE_PLAY_STATE_PLAYING)
  luaunit.assertEquals(snap2.rec.state, C.TAPE_REC_STATE_RECORDING)
  luaunit.assertEquals(type(snap2.play.loop), 'boolean') -- ensure play loop is boolean value
  luaunit.assertTrue(audio.tape_is_recording())
end

function TestAudioTape.test_on_status_normalizes_loop_flag_types()
  local env = setup_core_audio_env()
  local audio = env.audio
  local C = audio.tape.constants

  -- start with loop true
  audio.tape._on_status(C.TAPE_PLAY_STATE_READY, 0, 0, C.TAPE_REC_STATE_READY, 0, 1)
  luaunit.assertTrue(audio.tape.get_state().play.loop)

  -- numeric 0 should disable
  audio.tape._on_status(C.TAPE_PLAY_STATE_READY, 0, 0, C.TAPE_REC_STATE_READY, 0, 0)
  luaunit.assertFalse(audio.tape.get_state().play.loop)
end

function TestAudioTape.test_on_play_rec_file_extracts_filename()
  local env = setup_core_audio_env()
  local audio = env.audio

  -- test play file - should extract just the filename
  audio.tape._on_play_file('/some/dir/fileA.wav')
  local snap = audio.tape.get_state()
  luaunit.assertEquals(snap.play.file, 'fileA.wav')

  -- test rec file - should extract just the filename
  audio.tape._on_rec_file('/another/place/takeB.wav')
  local snap2 = audio.tape.get_state()
  luaunit.assertEquals(snap2.rec.file, 'takeB.wav')

  -- test with different path separators and extensions
  audio.tape._on_play_file('/path/to/my/recording.aiff')
  audio.tape._on_rec_file('/very/long/nested/path/to/file/output.wav')
  local snap3 = audio.tape.get_state()
  luaunit.assertEquals(snap3.play.file, 'recording.aiff')
  luaunit.assertEquals(snap3.rec.file, 'output.wav')

  -- clear on nil
  audio.tape._on_play_file(nil)
  audio.tape._on_rec_file(nil)
  local snap4 = audio.tape.get_state()
  luaunit.assertIsNil(snap4.play.file)
  luaunit.assertIsNil(snap4.rec.file)
end

function TestAudioTape.test_controls_call_engine()
  -- verify tape controls forward calls to engine with correct arguments
  local env = setup_core_audio_env()
  local audio = env.audio

  -- play controls
  _norns.tape_play_open.reset()
  audio.tape_play_open('/path/to.wav')
  luaunit.assertTrue(_norns.tape_play_open.called())
  luaunit.assertEquals(_norns.tape_play_open.args(1)[1], '/path/to.wav')

  _norns.tape_play_start.reset()
  audio.tape_play_start()
  luaunit.assertTrue(_norns.tape_play_start.called())

  _norns.tape_play_pause.reset()
  audio.tape_play_pause(true)
  luaunit.assertEquals(_norns.tape_play_pause.args(1)[1], 1)

  _norns.tape_play_pause.reset()
  audio.tape_play_pause(false)
  luaunit.assertEquals(_norns.tape_play_pause.args(1)[1], 0)

  _norns.tape_play_stop.reset()
  audio.tape_play_stop()
  luaunit.assertTrue(_norns.tape_play_stop.called())

  _norns.tape_play_loop.reset()
  audio.tape_play_loop(false)
  luaunit.assertEquals(_norns.tape_play_loop.args(1)[1], 0)

  -- record controls
  _norns.tape_record_open.reset()
  audio.tape_record_open('/rec.wav')
  luaunit.assertEquals(_norns.tape_record_open.args(1)[1], '/rec.wav')

  _norns.tape_record_start.reset()
  audio.tape_record_start()
  luaunit.assertTrue(_norns.tape_record_start.called())

  _norns.tape_record_pause.reset()
  audio.tape_record_pause(true)
  luaunit.assertEquals(_norns.tape_record_pause.args(1)[1], 1)

  _norns.tape_record_pause.reset()
  audio.tape_record_pause(false)
  luaunit.assertEquals(_norns.tape_record_pause.args(1)[1], 0)

  _norns.tape_record_stop.reset()
  audio.tape_record_stop()
  luaunit.assertTrue(_norns.tape_record_stop.called())
end

function TestAudioTape.test_compute_diskfree_invariants()
  local env = setup_core_audio_env({ disk = 1000 })
  local audio = env.audio

  -- returns an integer number of seconds
  local df_default = audio.tape_compute_diskfree()
  luaunit.assertEquals(type(df_default), 'number')
  luaunit.assertEquals(df_default, math.floor(df_default))

  -- smaller reserve yields more seconds; larger reserve yields fewer
  local df_low_reserve = audio.tape_compute_diskfree(100)
  local df_high_reserve = audio.tape_compute_diskfree(300)
  luaunit.assertTrue(df_low_reserve > df_high_reserve)

  -- more disk space yields more seconds
  local original_disk = norns.disk
  norns.disk = 2000
  local df_more_disk = audio.tape_compute_diskfree()
  luaunit.assertTrue(df_more_disk > df_default)
  norns.disk = original_disk

  -- handles missing norns/disk gracefully
  local saved_norns = norns
  norns = nil
  luaunit.assertEquals(audio.tape_compute_diskfree(), 0)
  norns = saved_norns
end
