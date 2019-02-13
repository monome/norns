local clock = {}

clock.threads = {}

clock.run = function(f)
  local coro = coroutine.create(f)
  local thread_id = #clock.threads + 1
  clock.threads[thread_id] = coro
  _clock_schedule(thread_id, 0)
end

clock.sync = function(...)
  return coroutine.yield(...)
end

-- todo: use c api instead
clock.resume = function(thread_id)
  result, time = coroutine.resume(clock.threads[thread_id])
  if result then
    _clock_schedule(thread_id, time)
  end
end

return clock
