--- clocked pattern recorder library
-- @module lib.reflection

local reflection = {}
reflection.__index = reflection

--- constructor
function reflection.new()
  local p = {}
  setmetatable(p, reflection)
  p.rec   = 0
  p.play  = 0
  p.event = {}
  p.step  = 0
  p.count = 0
  p.loop = 0
  p.clock = nil
  p.queued_rec  = nil
  p.rec_dur = nil
  p.quantize = 1/48
  p.endpoint = 0
  p.start_callback = function() end
  p.end_callback = function() end
  p.process = function(_) end
  return p
end

--- start transport
function reflection:start(quantization)
  quantization = quantization or self.quantize
  if self.clock then
    clock.cancel(self.clock)
  end
  self.clock = clock.run(function()
    clock.sync(quantization)
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
function reflection:set_rec(rec, dur)
  self.rec = rec == 1 and 1 or 0
  self.queued_rec = nil
  -- if standard rec flag is enabled but play isn't,
  --   then we should start playing, yeah?
  if rec == 1 and self.play == 0 then
    self:start()
  end
  if rec == 1 and dur then self.rec_dur = dur end
  if rec == 2 then
    if self.count > 0 then
      local fn = self.start_callback
      self.start_callback = function()
        -- on next data pass, enable recording,
        self:set_rec(1)
        -- call our callback
        fn()
        -- and restore the state of the callback
        self.start_callback = fn
      end
    else
      self.queued_rec = {state = true, duration = dur}
    end
  end
  if self.rec == 0 then self:_clear_flags() end
end

--- enable / disable looping
-- (has no effect on first run)
-- @tparam number loop 1 for looping or 0 for not looping
function reflection:set_loop(loop)
  self.loop = loop == 0 and 0 or 1
end

--- quantize playback
-- @tparam float q defaults to 1/48
-- (should be at least 1/96)
function reflection:set_quantization(q)
  self.quantize = q == nil and 1/48 or q
end

--- reset
function reflection:clear()
  if self.clock then
    clock.cancel(self.clock)
  end
  self.rec    = 0
  self.play   = 0
  self.event  = {}
  self.step   = 0
  self.count  = 0
  self.quantize = 1/48
  self.endpoint = 0
  self.queued_rec = nil
end

--- watch
function reflection:watch(event)
  local step_one = false
  if self.queued_rec ~= nil then
    self:set_rec(1,self.queued_rec.duration)
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

function reflection:begin_playback()
  self.step = 0
  self.play = 1
  self.clock = clock.run(function()
    self.start_callback()
    while self.play == 1 do
      clock.sync(1/96)
      self.step = self.step + 1
      local q = math.floor(96 * self.quantize)
      if self.endpoint == 0 then
        -- don't process on first pass
        if self.rec_dur then
          self.rec_dur = self.rec_dur - 1/96
          if self.rec_dur < 0 then
            self:set_rec(0)
            self:end_playback(true)
            self.rec_dur = nil
            if self.loop == 1 then
              self.start_callback()
              self.step = 0
              self.play = 1
            end
          end
        end
      else
        if self.step % q ~= 1 then goto continue end
        for i = q - 1, 0, - 1 do
          if self.event[self.step - i] and next(self.event[self.step - i]) then
            for j = 1, #self.event[self.step - i] do
              local event = self.event[self.step - i][j]
              if not event._flag then self.process(event) end
            end
          end
        end
        if self.rec_dur then
          self.rec_dur = self.rec_dur - 1/96
          if self.rec_dur > 0 then goto continue end
          self:set_rec(0)
          -- as a convenience, if this was our first pass, stop playback
          if self.endpoint == 0 then self:end_playback() return end
        end
        ::continue::
        if self.count > 0 and self.step >= self.endpoint then
          if self.loop == 0 then
            self:end_playback()
          elseif self.loop == 1 then
            self.step = self.step - self.endpoint
            self:_clear_flags()
            self:start_callback()
          end
        end
      end
    end
  end)
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

local function deep_copy(tbl)
  local ret = {}
  if type(tbl) ~= 'table' then return tbl end
  for key, value in pairs(tbl) do
    ret[key] = deep_copy(value)
  end
  return ret
end

--- copy data from one reflection to another
-- @tparam table to reflection to copy to
-- @tparam table from reflection to copy from
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
end

return reflection
