-- lua/test/core/clock_test.lua
-- unit tests for clock coroutine cancellation

local luaunit = require('lib/test/luaunit')

local function setup_clock()
  local calls = {cancelled = {}, sleeps = {}}
  _norns = {
    clock = {},
    clock_cancel = function(id) table.insert(calls.cancelled, id) end,
    clock_schedule_sleep = function(id, time) table.insert(calls.sleeps, {id, time}) end,
    clock_schedule_sync = function() end,
  }
  package.loaded['core/clock'] = nil
  return require('core/clock'), calls
end

TestClock = {}

function TestClock.test_late_resume_of_cancelled_clock_is_ignored()
  local clock, calls = setup_clock()
  local resumed = false
  local id = clock.run(function()
    clock.sleep(0.1)
    resumed = true
  end)

  luaunit.assertEquals(#calls.sleeps, 1)
  clock.cancel(id)
  luaunit.assertNil(clock.threads[id])
  luaunit.assertTrue(pcall(clock.resume, id))
  luaunit.assertFalse(resumed)
end

function TestClock.test_unknown_clock_id_still_errors()
  local clock = setup_clock()
  local id = clock.run(function() clock.sleep(0.1) end)
  luaunit.assertFalse(pcall(clock.resume, id + 1))
end

function TestClock.test_active_clock_resumes_normally()
  local clock = setup_clock()
  local resumed = false
  local id = clock.run(function()
    clock.sleep(0.1)
    resumed = true
  end)

  clock.resume(id)
  luaunit.assertTrue(resumed)
  luaunit.assertNil(clock.threads[id])
end

function TestClock.test_active_clock_errors_still_propagate()
  local clock = setup_clock()
  local ok, err = pcall(clock.run, function() error('clock body failed') end)
  luaunit.assertFalse(ok)
  luaunit.assertStrContains(err, 'clock body failed')
end
