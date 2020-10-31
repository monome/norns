-- midi helper module
-- @module process
-- @alias process

local util = require('util')
local Deque = require('container/deque')
local DefaultTable = require('container/defaulttable')

sky.use('core/object')

--
-- Device class
--
local Device = sky.Object:extend()

function Device:new(props)
  Device.super.new(self)
  -- for k,v in pairs(props) do
  --   self[k] = v
  -- end
  if props ~= nil then
    self.bypass = props.bypass or false
  else
    self.bypass = false
  end
end

function Device:device_inserted(chain)
  -- nothing to do
end

function Device:device_removed(chain)
  -- nothing to do
end

function Device:process(event, output, state)
  if self.bypass then return end
  output(event)
end

--
-- InputBase class (event source)
--
local InputBase = sky.Object:extend()

function InputBase:new(props)
  InputBase.super.new(self, props)
  self.chain = props.chain
  self.enabled = props.enabled or true
end

--
-- [midi] Input class (event source)
--
local Input = InputBase:extend()

function Input:new(props)
  Input.super.new(self, props)

  self._notes = DefaultTable.new(0)

  -- determine which device to use
  local d = props.device
  if d == nil then
    if props.name then
      -- attempt to find the midi device by name
      for i,v in ipairs(midi.vports) do
        if sky.starts_with(v.name, props.name) then
          d = midi.connect(i)
          self.name = props.name
        end
      end
    else
      d = midi.connect(1)
    end
  end

  self:set_device(d)

  if self.device == nil then
    local n = self.name or "<none>"
    print("warning: input not connected to device " .. n)
  end
end

function Input:set_device(device)
  if device ~= nil then
    -- clean up callbacks if need be
    if self.device ~= nil then
      self.device.event = nil
    end
    self.device = device
    -- install device event handler
    self.device.event = function(data)
      self:on_midi_event(data)
    end
  end
end

function Input:on_midi_event(data)
  if not self.enabled or self.chain == nil then
    -- nothing to do
    return
  end

  local event = midi.to_msg(data)
  if event ~= nil then
    if sky.is_type(event, sky.types.NOTE_ON) then
      self:_add_note_correlation(event)
    elseif sky.is_type(event, sky.types.NOTE_OFF) then
      self:_pair_note_correlation(event)
    end
    self.chain:process(event)
  end
end

function Input:_add_note_correlation(event)
  local id = sky.note_id(event)
  if event.vel == 0 then
    self:_pair_note_correlation(event)
  else
    local instance = self._notes[id]
    self._notes[id] = instance + 1
    event.correlation = sky.note_cid(event, instance)
  end
end

function Input:_pair_note_correlation(event)
  local id = sky.note_id(event)
  local instance = self._notes[id]
  if instance > 0 then
    instance = instance - 1
    self._notes[id] = instance
  end
  event.correlation = sky.note_cid(event, instance)
end


-- allow this input to invoke callbacks
function Input:enable()
  self.enabled = true
end

-- temporarily stop this input from invoking callbacks
function Input:disable()
  self.enabled = false
end

-- perminantly remove this input from receiving further events
function Input:cleanup()
  self:disable()
  if self.device and self.device.cleanup then
    self.device:cleanup()
  end
end

--
-- Output class (event sink)
--
local Output = Device:extend()

function Output:new(props)
  Output.super.new(self, props)

  -- determine which device to use
  local d = props.device
  if d == nil then
    if props.name then
      -- attempt to find the midi device by name
      for i,v in ipairs(midi.vports) do
        if sky.starts_with(v.name, props.name) then
          d = midi.connect(i)
          self.name = props.name
        end
      end
    else
      d = midi.connect(2)
    end
  end

  self:set_device(d)

  if self.device == nil then
    local n = self.name or "<none>"
    print("warning: output not connected to device " .. n)
  end

  if type(props.enabled) ~= "boolean" then
    self.enabled = true
  end
end

function Output:set_device(device)
  self.device = device
end

function Output:process(event, output)
  local t = event.type
  if self.enabled and self.device and (t ~= nil) then
    -- filter out non-midi events
    if sky.type_names[t] ~= nil then  -- FIXME: find a better test than this
      self.device:send(event)
    end
  end

  -- pass events on
  output(event)
end

--
-- Scheduler (of chain device callback events)
--

local Scheduler = {}
Scheduler.__index = Scheduler

function Scheduler.new(chain, device_index)
  local o = setmetatable({}, Scheduler)
  o.chain = chain
  o.device_index = device_index
  o.clock_pool = Deque.new()
  o.clock_id = nil
  return o
end

local _scheduler_coro = function(self, when, event, method)
  method(when)
  self.chain:process(event, self.device_index)
end

function Scheduler:sync(when, event)
  clock.run(_scheduler_coro, self, when, event, clock.sync)
end

function Scheduler:sleep(when, event)
  clock.run(_scheduler_coro, self, when, event, clock.sleep)
end

function Scheduler:now(event)
  self.chain:process(event, self.device_index)
end

function Scheduler:run(coro, ...)
  self:cancel()
  local output = function(event)
    self.chain:process(event, self.device_index)
  end
  self.clock_id = clock.run(coro, output)
end

function Scheduler:cancel()
  if self.clock_id ~= nil then
    clock.cancel(self.clock_id)
    self.clock_id = nil
  end
end

--
-- Chain class
--
local Chain = {}
Chain.__index = Chain

function Chain.new(devices)
  local o = setmetatable({}, Chain)
  o.bypass = false
  o.devices = devices or {}

  o._state = { process_count = 0 }
  o._buffers = { Deque.new(), Deque.new() }
  o._schedulers = {}

  -- rip through devices and if there are functions wrap them in a
  -- generic processor object which supports bypass etc.
  for i, d in ipairs(o.devices) do
    if type(d) == 'function' then
      d = sky.Func(d)
      o.devices[i] = d
    end
    -- handle insertion callback
    d:device_inserted(o)
  end

  return o
end

function Chain:init()
  self:process(sky.mk_script_init())
end

function Chain:redraw()
  self:process(sky.mk_script_redraw())
end

function Chain:cleanup()
  self:process(sky.mk_script_cleanup())
end

function Chain:process(event, from_device, parent_output)
  -- print('chain:process', sky.to_string(event), from_device)
  if self.bypass then
    return
  end

  local state = self._state
  state.process_count = state.process_count + 1

  local source = self._buffers[1]
  local sink = self._buffers[2]

  return self._process(event, state, self.devices, source, sink, from_device, parent_output)
end

local function ipairs_from(tbl, start)
  local iter = function(tbl, i)
    i = i + 1
    local v = tbl[i]
    if v ~= nil then return i, v end
  end
  local initial = 0
  if start then initial = start - 1 end
  return iter, tbl, initial
end

function Chain._process(event, state, devices, source, sink, from_device, parent_output)
  source:clear()
  sink:clear()

  local output = function(event)
    sink:push_back(event)
  end

  -- populate the source event queue with the event to process
  source:push_back(event)

  for i, processor in ipairs_from(devices, from_device) do

    event = source:pop()
    while event do
      -- print("\ndevice:", i, "event:", event, "processor:", processor)
      processor:process(event, output, state)
      event = source:pop()
      -- print("sink c:", sink:count())
    end

    -- swap input/output buffers
    local t = source
    source = sink
    sink = t

    -- event = source:pop()
    if source:count() == 0 then
      -- no more events to process, end chain processing early
      -- print("breaking out of process loop")
      break
    end

    -- output to parent chain if an output was provided
    if parent_output ~= nil then
      event = source:pop()
      while event do
        parent_output(event)
        event = source:pop()
      end
    end
  end

  -- return output buffer of last processor
  return source
end

function Chain:run(events)
  local output = Deque.new()
  for i, ein in ipairs(events) do
    local r = self:process(ein)
    if r ~= nil then
      -- flatten output
      output:extend_back(r)
    end
  end
  return output:to_array()
end

function Chain:scheduler(device)
  local s = self._schedulers[device]
  if s == nil then
    -- location the position of the device within the chain so processing can
    -- start there
    local device_index = 0
    for i, d in ipairs(self.devices) do
      if d == device then
        device_index = i
        break
      end
    end
    if device_index < 1 then
      error('device not a member of this chain')
    end
    s = Scheduler.new(self, device_index)
    self._schedulers[device] = s
  end
  return s
end

--
-- Group
--
local Group = Device:extend()

function Group:new(props)
  Group.super.new(self, props)
  self.source = Deque.new()
  self.sink = Deque.new()
  self.devices = {}
  for _, v in ipairs(props) do
    table.insert(self.devices, v)
  end
end

function Group:process(event, output, state)
  if self.bypass then
    output(event)
  else
    -- process children in the same manner as a chain then output all the results
    local results = Chain._process(event, state, self.devices, self.source, self.sink)
    for _, v in results:ipairs() do
      output(v)
    end
  end
end

--
-- module
--

return {
  -- objects
  Device = Device,
  InputBase = InputBase,
  Input = Input,
  Output = Output,
  Chain = Chain.new,
  Group = Group,

  -- debug
  __input_count = input_count,
  __inputs = inputs,
}
