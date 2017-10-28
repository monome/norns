-- Poll class

Poll = {}
Poll.__index = Poll


function Poll:new(idx, name, pollType)
   local p = setmetatable({}, Poll)
   p.idx = idx
   p.name = name
   p.period = 1
   p.type = pollType
   p.callback = function() end
end

function Poll:start()
   start_poll(self.idx)
end

function Poll:stop()
   stop_poll(self.idx)
end

function Poll:set_time(t)
   self.period = t
   set_poll_time(self.idx, t)
end
