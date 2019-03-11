local clock = {}

clock.threads = {}

clock.run = function(f)
  local coro = coroutine.create(f)
  local thread_id = #clock.threads + 1
  clock.threads[thread_id] = coro
  _clock_schedule_sleep(thread_id, 0)
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
clock.resume = function(thread_id)
  result, mode, time = coroutine.resume(clock.threads[thread_id])
  if result then
    if mode == SLEEP then
      _clock_schedule_sleep(thread_id, time)
    else
      _clock_schedule_sync(thread_id, time)
    end
  end
end

return clock
