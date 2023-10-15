--- clocked pattern recorder library
-- @module lib.reflection

local reflection = {}
reflection.__index = reflection

--- constructor
function reflection.new()
  local p = {}
  setmetatable(p, reflection)
  p.rec                  = 0
  p.rec_enabled          = 0
  p.play                 = 0
  p.event                = {}
  p.event_prev           = {}
  p.step                 = 0
  p.count                = 0
  p.loop                 = 0
  p.clock                = nil
  p.queued_rec           = nil
  p.rec_dur              = nil
  p.quantize             = 1 / 48
  p.endpoint             = 0
  p.start_callback       = function() end
  p.end_of_loop_callback = function() end
  p.end_of_rec_callback  = function() end
  p.end_callback         = function() end
  p.process              = function(_) end
  return p
end

local function deep_copy(tbl)
  local ret = {}
  if type(tbl) ~= 'table' then return tbl end
  for key, value in pairs(tbl) do
    ret[key] = deep_copy(value)
  end
  return ret
end

--- copy data from one reflection to another
function reflection.copy(to, from)
  to.event = deep_copy(from.event)
  to.endpoint = from.endpoint
end

--- doubles the current loop
function reflection:double()
  local copy = deep_copy(self.event)
  for i = 1, self.endpoint do
    self.event[self.endpoint + i] = copy[i]
  end
  self.endpoint = self.endpoint * 2
  self.step_max = self.endpoint
end

--- start transport
-- @tparam number beat_sync (optional) sync playback start to beat value
function reflection:start(beat_sync)
  beat_sync = beat_sync or self.quantize
  if self.clock then
    clock.cancel(self.clock)
  end
  self.clock = clock.run(function()
    clock.sync(beat_sync)
    self:begin_playback()
  end)
end

--- stop transport
function reflection:stop()
  if self.clock then
    clock.cancel(self.clock)
  end
  self.clock = clock.run(function()
    clock.sync(self.quantize)
    self:end_playback()
  end)
end

--- enable / disable record head
-- @tparam number rec 1 for recording, 2 for queued recording or 0 for not recording
-- @tparam number dur (optional) duration in beats for recording
-- @tparam number beat_sync (optional) sync recording start to beat value
function reflection:set_rec(rec, dur, beat_sync)
  self.rec = rec == 1 and 1 or 0
  self.rec_enabled = rec > 0 and 1 or 0
  self.queued_rec = nil
  -- if standard rec flag is enabled but play isn't,
  --   then we should start playing, yeah?
  if rec == 1 and self.play == 0 then
    self:start(beat_sync)
  end
  if rec == 1 and self.count > 0 then
    self.event_prev = {}
    self.event_prev = deep_copy(self.event)
  end
  if rec == 1 and dur then
    self.rec_dur = { count = dur, length = dur }
  end
  if rec == 2 then
    if self.count > 0 then
      local fn = self.start_callback
      self.start_callback = function()
        -- on next data pass, enable recording,
        self:set_rec(1, dur)
        -- call our callback
        fn()
        -- and restore the state of the callback
        self.start_callback = fn
      end
    else
      self.queued_rec = { state = true, duration = dur }
    end
  end
  if rec == 0 then
    self:_clear_flags()
    self.end_of_rec_callback()
  end
end

--- enable / disable looping
-- @tparam number loop 1 for looping or 0 for not looping
function reflection:set_loop(loop)
  self.loop = loop == 0 and 0 or 1
end

--- quantize playback
-- @tparam float q defaults to 1/48
-- (should be at least 1/96)
function reflection:set_quantization(q)
  self.quantize = q == nil and 1 / 48 or q
end

--- change pattern length in beats
-- @tparam number beats
function reflection:set_length(beats)
  if self.count > 0 then
    self.endpoint = beats * 96
  end
end

--- undo previous overdub
function reflection:undo()
  if next(self.event_prev) then
    self.event = deep_copy(self.event_prev)
  end
end

--- reset
function reflection:clear()
  if self.clock then
    clock.cancel(self.clock)
  end
  self.rec = 0
  self.play = 0
  self.event = {}
  self.event_prev = {}
  self.step = 0
  self.count = 0
  self.endpoint = 0
  self.queued_rec = nil
end

--- watch
function reflection:watch(event)
  local step_one = false
  if self.queued_rec ~= nil then
    self:set_rec(1, self.queued_rec.duration)
    self.queued_rec = nil
    step_one = true
  end
  if (self.rec == 1 and self.play == 1) or step_one then
    event._flag = true
    local s = math.floor(step_one == true and 1 or self.step)

    if not self.event[s] then
      self.event[s] = {}
    end
    table.insert(self.event[s], event)
    self.count = self.count + 1
  end
end

-- must be called from within a clock.run
function reflection:begin_playback()
  self.step = 0
  self.play = 1
  self.start_callback()
  while self.play == 1 do
    clock.sync(1 / 96)
    self.step = self.step + 1
    local q = math.floor(96 * self.quantize)
    if self.endpoint == 0 then
      -- don't process on first pass
      if self.rec_dur then
        self.rec_dur.count = self.rec_dur.count - 1 / 96
        if self.rec_dur.count <= 0 then
          self.endpoint = self.rec_dur.length * 96
          self:set_rec(0)
          self.rec_dur = nil
          if self.loop == 1 then
            self.start_callback()
            self.step = 0
            self.play = 1
          end
        end
      else
        if self.rec == 0 and self.count > 0 then
          self.endpoint = self.step
          if self.loop == 1 then
            self.step = 0
            self:_clear_flags()
            self:start_callback()
          end
        end
      end
    else
      if self.step % q ~= 1 then goto continue end
      for i = q - 1, 0, -1 do
        if self.event[self.step - i] and next(self.event[self.step - i]) then
          for j = 1, #self.event[self.step - i] do
            local event = self.event[self.step - i][j]
            if not event._flag then self.process(event) end
          end
        end
      end
      ::continue::
      if self.rec_dur then
        self.rec_dur.count = self.rec_dur.count - 1 / 96
        if self.rec_dur.count <= 0 then
          self:set_rec(0)
          self.rec_dur = nil
        end
      end
      -- if the endpoint is reached reset counter or stop playback
      if self.count > 0 and self.step >= self.endpoint then
        self.end_of_loop_callback()
        if self.loop == 0 then
          self:end_playback()
        elseif self.loop == 1 then
          self.step = 0
          self:_clear_flags()
          self:start_callback()
        end
      end
    end
  end
end

function reflection:end_playback(silent)
  if self.clock and not silent then
    clock.cancel(self.clock)
    self.clock = nil
  end
  self.play = 0
  self.rec = 0
  if self.endpoint == 0 and next(self.event) then
    self.endpoint = self.step
  end
  self:_clear_flags()
  self.end_callback()
end

function reflection:_clear_flags()
  if self.endpoint == 0 then return end
  for i = 1, self.endpoint do
    local list = self.event[i]
    if list then
      for _, event in ipairs(list) do
        event._flag = nil
      end
    end
  end
end

return reflection
