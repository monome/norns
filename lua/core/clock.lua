local clock = {}

clock.threads = {}

clock.run = function(f)
  local coro = coroutine.create(f)
  local coro_id = #clock.threads + 1
  clock.threads[coro_id] = coro
  _clock_schedule_sleep(coro_id, 0)
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
  result, mode, time = coroutine.resume(clock.threads[coro_id])
  if result then
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

return clock
