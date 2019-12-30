--- high-resolution metro API
-- @classmod metro
-- @alias Metro_mt

local Metro = {}
Metro.__index = Metro

Metro.num_metros = 36
Metro.num_script_metros = 30 -- 31-35 are reserved

Metro.metros = {}
Metro.available = {}
Metro.assigned = {}


--- initialize a metro.
-- assigns unused id.
-- @tparam function arg callback function
-- @tparam number arg_time time period between ticks (seconds).
-- @tparam number arg_count number of ticks. infinite by default.
function Metro.init(arg, arg_time, arg_count)
  local event = 0
  local time = arg_time or 1
  local count = arg_count or -1

  if type(arg) == "table" then
    event = arg.event
    time = arg.time or 1
    count = arg.count or -1
  else
    event = arg
  end

  local id = nil
  for i, val in pairs(Metro.available) do
    if val == true then
      id = i
      break
    end
  end
  if id ~= nil then
    Metro.assigned[id] = true
    Metro.available[id] = false
    local m = Metro.metros[id]
    m.event = event
    m.time = time
    m.count= count
    return m
  end
  print("metro.init: nothing available")
  return nil
end

--- free
-- @tparam number id
function Metro.free(id)
  Metro.metros[id]:stop()
  Metro.available[id] = true
  Metro.assigned[id] = false
end

--- free all
function Metro.free_all()
  for i=1,Metro.num_script_metros do
    Metro.free(i)
  end
end


--- constructor.
-- @tparam integer id : identifier
-- @treturn Metro
function Metro.new(id)
  local m = {}
  m.props = {
    id = id,
    time = 1,
    count = -1,
    event = nil,
    init_stage = 1
  }
  setmetatable(m, Metro)
  return m
end

--- start a metro.
-- @tparam[opt] number time - time period between ticks (seconds.) by default, re-use the last period
-- @tparam[opt] number count - number of ticks. infinite by default
-- @tparam[opt] number stage - initial stage number (1-based.) 1 by default
function Metro:start(time, count, stage)
  if type(time) == "table" then
    if time.time then self.props.time = time.time end
    if time.count then self.props.count = time.count end
    if time.stage then self.props.stage = time.stage end
  else

    if time then self.props.time = time end
    if count then self.props.count = count end
    if stage then self.init_stage = stage end
  end
  self.is_running = true
  _norns.metro_start(self.props.id, self.props.time, self.props.count, self.props.init_stage) -- C function
end

--- stop a metro.
function Metro:stop()
  _norns.metro_stop(self.props.id) -- C function
  self.is_running = false
end


Metro.__newindex = function(self, idx, val)
  if idx == "time" then
    self.props.time = val
    -- NB: metro time isn't applied until the next wakeup.
    -- this is true even if you are setting time from the metro callback;
    -- metro has already gone to sleep when lua main thread gets
    -- if you need a fully dynamic metro, re-schedule on the wakeup
    _norns.metro_set_time(self.props.id, self.props.time)
  elseif idx == 'count' then self.props.count = val
  elseif idx == 'init_stage' then self.props.init_stage = val
  else -- FIXME: dunno if this is even necessary / a good idea to allow
    rawset(self, idx, val)
  end
end

-- class custom .__index.
-- [] accessor returns one of the static metro objects.
Metro.__index = function(self, idx)
  if type(idx) == "number" then
    return Metro.metros[idx]
  elseif idx == "start" then return Metro.start
  elseif idx == "stop" then return Metro.stop
  elseif idx == 'id' then return self.props.id
  elseif idx == 'count' then return self.props.count
  elseif idx == 'time' then return self.props.time
  elseif idx == 'init_stage' then return self.props.init_stage
  elseif self.props.idx then
    return self.props.idx
  else
    return rawget(self, idx)
  end
end

setmetatable(Metro, Metro)

--
-- static initialization

-- initialize module data
for i=1,Metro.num_metros do
  Metro.metros[i] = Metro.new(i)
end

for i=1,Metro.num_script_metros do
  Metro.available[i] = true
  Metro.assigned[i] = false
end



--- Global Functions
-- @section globals

--- callback on metro tick from C.
_norns.metro = function(idx, stage)
  if Metro.metros[idx] then
    if Metro.metros[idx].event then
      Metro.metros[idx].event(stage)
    end
  end
end



return Metro
