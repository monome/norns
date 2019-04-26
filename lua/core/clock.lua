local clock = {}

clock.threads = {}

clock.run = function(f)
  local coro = coroutine.create(f)
  local coro_id = #clock.threads + 1
  clock.threads[coro_id] = coro
  clock.resume(coro_id)
  return coro_id
end

local SLEEP = 0
local SYNC = 1

clock.sleep = function(...)
  return coroutine.yield(SLEEP, ...)
end

clock.sync = function(...)
  return coroutine.yield(SYNC, ...)
end

-- todo: use c api instead
clock.resume = function(coro_id)
  coro = clock.threads[coro_id]

  if coro == nil then
    return -- todo: report error
  end

  result, mode, time = coroutine.resume(clock.threads[coro_id])

  if coroutine.status(coro) ~= "dead" and result and mode ~= nil then
    if mode == SLEEP then
      _clock_schedule_sleep(coro_id, time)
    else
      _clock_schedule_sync(coro_id, time)
    end
  end
end

clock.stop = function(coro_id)
  _clock_cancel(coro_id)
  clock.threads[coro_id] = nil
end

clock.cleanup = function()
  for i = 1, #clock.threads do
    coro = clock.threads[i]
    if coro then
      clock.stop(i)
    end
  end
end

clock.INTERNAL = 0
clock.MIDI = 1

clock.set_source = function(source)
  _clock_set_source(source)
end

clock.get_time_beats = function()
  return _clock_get_time_beats()
end

return clock
