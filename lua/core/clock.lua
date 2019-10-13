--- clock coroutines
-- @module clock

local clock = {}

clock.threads = {}

local clock_id_counter = 1
local function new_id()
  local id = clock_id_counter
  clock_id_counter = clock_id_counter + 1
  return id
end

--- create a coroutine from the given function and immediately run it;
-- the function parameter is a task that will suspend when clock.sleep and clock.sync are called inside it and will wake up again after specified time.
-- @tparam function f
-- @treturn integer : coroutine ID that can be used to stop it later
clock.run = function(f)
  local coro = coroutine.create(f)
  local coro_id = new_id()
  clock.threads[coro_id] = coro
  clock.resume(coro_id)
  return coro_id
end

local SLEEP = 0
local SYNC = 1


--- yield and schedule waking up the coroutine in s seconds;
-- must be called from within a coroutine started with clock.run.
-- @tparam float s : seconds
clock.sleep = function(...)
  return coroutine.yield(SLEEP, ...)
end


--- yield and schedule waking up the coroutine at beats beat;
-- the coroutine will suspend for the time required to reach the given fraction of a beat;
-- must be called from within a coroutine started with clock.run.
-- @tparam float beats : next fraction of a beat at which the coroutine will be resumed. may be larger than 1.
clock.sync = function(...)
  return coroutine.yield(SYNC, ...)
end

-- todo: use c api instead
clock.resume = function(coro_id)
  local coro = clock.threads[coro_id]

  if coro == nil then
    return -- todo: report error
  end

  local result, mode, time = coroutine.resume(clock.threads[coro_id])

  if coroutine.status(coro) == "dead" and result == false then
    error(mode)
  end

  if coroutine.status(coro) ~= "dead" and result and mode ~= nil then
    if mode == SLEEP then
      _clock_schedule_sleep(coro_id, time)
    else
      _clock_schedule_sync(coro_id, time)
    end
  end
end

--- stop execution of a coroutine started using clock.run.
-- @tparam integer coro_id : coroutine ID
clock.stop = function(coro_id)
  _clock_cancel(coro_id)
  clock.threads[coro_id] = nil
end


clock.cleanup = function()
  for id, coro in pairs(clock.threads) do
    if coro then
      clock.stop(id)
    end
  end
end

clock.INTERNAL = 0
clock.MIDI = 1

--- select the sync source, currently clock.INTERNAL and clock.MIDI.
-- @tparam integer source : clock.INTERNAL (0) or clock.MIDI (1)
clock.set_source = function(source)
  _clock_set_source(source)
end

clock.get_time_beats = function()
  return _clock_get_time_beats()
end

return clock
