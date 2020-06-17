local Observers = {}

local function _add(name, receiver)
  local existing = Observers[name]
  if existing == nil then
    existing = {}
    Observers[name] = existing
  end
  -- set like behavior
  local listening = false
  for _, r in ipairs(existing) do
    if r == receiver then
      listening = true
      break
    end
  end
  if not listening then
    table.insert(existing, receiver)
  end
end

local function _remove(name, receiver)
  local existing = Observers[name]
  if existing then
    table.remove(existing, receiver)
  end
end

--
-- Receive
--

local Receive = sky.Device:extend()

function Receive:new(sender_name)
  Receive.super.new(self)
  self.from = sender_name
  _add(self.from, self)
  self._scheduler = nil
end

function Receive:device_inserted(chain)
  if self._scheduler ~= nil then
    error('Receive: one instance cannot be used in multiple chians at the same time')
  end
  self._scheduler = chain:scheduler(self)
end

function Receive:device_removed(chain)
  self._scheduler = nil
end

function Receive:inject(event)
  if not self.bypass and self._scheduler then
    --print('injecting;', self.from, self, sky.to_string(event), self._scheduler.device_index )
    self._scheduler:now(event)
  end
end

function Receive:process(event, output, state)
  output(event)
end

--
-- Send
--

local Send = sky.Device:extend()

function Send:new(name)
  Send.super.new(self)
  self.to = name
end

function Send:process(event, output, state)
  output(event) -- pass events through to this chain
  local listeners = Observers[self.to]
  if listeners then
    for _, r in ipairs(listeners) do
      r:inject(event)
    end
  end
end

--
-- Forward
--

local Forward = sky.Device:extend()

function Forward:new(chain)
  Forward.super.new(self)
  self.chain = chain
end

function Forward:process(event, output, state)
  output(event) -- pass events through to this chain
  if self.chain then
    self.chain:process(event)
  end
end

--
-- module
--

return {
  Receive = Receive,
  Send = Send,
  Forward = Forward,
}


