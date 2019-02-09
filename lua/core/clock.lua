local clock = {}

local coros = {}

clock.run = function(f)
  local coro = coroutine.create(f)
  coros[#coros + 1] = coro
  _clock_run(coro)
end

clock.sync = function(...)
  return coroutine.yield(...)
end

return clock
