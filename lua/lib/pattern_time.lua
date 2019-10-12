local pattern = {}
pattern.__index = pattern

function pattern.new()
  local i = {}
  setmetatable(i, pattern)
  i.rec = 0
  i.play = 0
  i.prev_time = 0
  i.event = {}
  i.time = {}
  i.count = 0
  i.step = 0
  i.time_factor = 1

  i.metro = metro.init(function() i:next_event() end,1,1)

  i.process = function(_) print("event") end

  return i
end

function pattern:clear()
  self.metro:stop()
  self.rec = 0
  self.play = 0
  self.prev_time = 0
  self.event = {}
  self.time = {}
  self.count = 0
  self.step = 0
  self.time_factor = 1
end

--- adjust the time factor of this pattern.
-- @param f time factor
function pattern:set_time_factor(f)
  self.time_factor = f or 1
end

function pattern:rec_start()
  print("pattern rec start")
  self.rec = 1
end

function pattern:rec_stop()
  if self.rec == 1 then
    self.rec = 0
    if self.count ~= 0 then
      print("count "..self.count)
      local t = self.prev_time
      self.prev_time = util.time()
      self.time[self.count] = self.prev_time - t
      --tab.print(self.time)
    else
      print("no events recorded")
    end 
  else print("not recording")
  end
end

function pattern:watch(e)
  if self.rec == 1 then
    self:rec_event(e)
  end
end

function pattern:rec_event(e)
  local c = self.count + 1
  if c == 1 then
    self.prev_time = util.time()
    --print("first event")
  else
    local t = self.prev_time
    self.prev_time = util.time()
    self.time[c-1] = self.prev_time - t
    --print(self.time[c-1])
  end
  self.count = c
  self.event[c] = e
end

function pattern:start()
  if self.count > 0 then
    print("start pattern ")
    self.process(self.event[1])
    self.play = 1
    self.step = 1
    self.metro.time = self.time[1] * self.time_factor
    self.metro:start()
  end
end

function pattern:next_event()
  if self.step == self.count then self.step = 1
  else self.step = self.step + 1 end
  --print("next step "..self.step)
  --event_exec(self.event[self.step])
  self.process(self.event[self.step])
  self.metro.time = self.time[self.step] * self.time_factor
  --print("next time "..self.metro.time)
  self.metro:start()
end

function pattern:stop()
  if self.play == 1 then
    print("stop pattern ")
    self.play = 0
    self.metro:stop()
  else print("not playing") end
end

return pattern
