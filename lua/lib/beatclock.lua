--- BeatClock
-- @classmod BeatClock

local BeatClock = {}
BeatClock.__index = BeatClock

--- constructor
-- @tparam string name
-- @treturn BeatClock
function BeatClock.new(name)
  local i = {}
  setmetatable(i, BeatClock)
  
  i.name = name or ""
  i.playing = false
  i.ticks_per_step = 6
  i.current_ticks = i.ticks_per_step - 1
  i.steps_per_beat = 4
  i.beats_per_bar = 4
  i.step = i.steps_per_beat - 1
  i.beat = i.beats_per_bar - 1
  i.external = false
  i.send = false
  i.midi = false
  
  i.metro = metro.init()
  i.metro.count = -1
  i.metro.event = function() i:tick() end
  i:bpm_change(110)

  i.on_step = function(e) print("BeatClock executing step") end
  i.on_start = function(e) end
  i.on_stop = function(e) end
  i.on_select_internal = function(e) print("BeatClock using internal clock") end
  i.on_select_external = function(e) print("BeatClock using external clock") end
  
  i:enable_midi()

  return i
end

--- start
function BeatClock:start(dev_id)
  self.playing = true
  if not self.external then
    self.metro:start()
  end
  self.current_ticks = self.ticks_per_step - 1
  if self.midi and self.send then
    for id, device in pairs(midi.devices) do
      if id ~= dev_id then
        device:send({251})
      end
    end
  end
  self.on_start()
end

--- stop
function BeatClock:stop(dev_id)
  self.playing = false
  self.metro:stop()
  if self.midi and self.send then
    for id, device in pairs(midi.devices) do
      if id ~= dev_id then
        device:send({252})
      end
    end
  end
  self.on_stop()
end

--- advance step
function BeatClock:advance_step()
  self.step = (self.step + 1) % self.steps_per_beat
  if self.step == 0 then
    self.beat = (self.beat + 1) % self.beats_per_bar
  end
  self.on_step()
end

--- tick
function BeatClock:tick(dev_id)
  self.current_ticks = (self.current_ticks + 1) % self.ticks_per_step
  if self.playing and self.current_ticks == 0 then
    self:advance_step()
  end
  
  if self.midi and self.send then
    for id, device in pairs(midi.devices) do
      if id ~= dev_id then
        device:send({248})
      end
    end
  end
end

--- reset
function BeatClock:reset(dev_id)
  self.step = self.steps_per_beat - 1
  self.beat = self.beats_per_bar - 1
  self.current_ticks = self.ticks_per_step - 1
  if self.midi and self.send then
    for id, device in pairs(midi.devices) do
      if id ~= dev_id then
        device:send({250})
        if not self.playing then -- force reseting while stopped requires a start/stop (??)
          device:send({252})
        end
      end
    end
  end
end

--- clock source change
function BeatClock:clock_source_change(source)
  self.current_ticks = self.ticks_per_step - 1
  if source == 1 then
    self.external = false
    if self.playing then
      self.metro:start()
    end
    self.on_select_internal()
  else
    self.external = true
    self.metro:stop()
    self.on_select_external()
  end
end

--- bpm change
function BeatClock:bpm_change(bpm)
  self.bpm = bpm
  self.metro.time = 60/(self.ticks_per_step * self.steps_per_beat * self.bpm)
end

--- add clock params
function BeatClock:add_clock_params()
  params:add_option("clock", "clock", {"internal", "external"}, self.external or 2 and 1)
  params:set_action("clock", function(x) self:clock_source_change(x) end)
  params:add_number("bpm", "bpm", 1, 480, self.bpm)
  params:set_action("bpm", function(x) self:bpm_change(x) end)
  params:add_option("clock_out", "clock out", { "no", "yes" }, self.send or 2 and 1)
  params:set_action("clock_out", function(x) if x == 1 then self.send = false else self.send = true end end)
end

--- enable midi
function BeatClock:enable_midi()
  self.midi = true  
end

--- process midi
function BeatClock:process_midi(data)
  if self.midi then
    local status = data[1]
    local data1 = data[2]
    local data2 = data[3]
  
    if self.external then 
      if status == 248 then -- midi clock
        self:tick(id)
      elseif status == 250 then -- midi clock start
        self:reset(id)
        self:start(id)
      elseif status == 251 then -- midi clock continue
        self:start(id)
      elseif status == 252 then -- midi clock stop
        self:stop(id)
      end
    end 
  end
end

  
return BeatClock
