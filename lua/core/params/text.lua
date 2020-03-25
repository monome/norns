-- Text class
-- @module Text

local Text = {}
Text.__index = Text

local tText = 8

function Text.new(id, name, text)
  local o = setmetatable({}, Text)
  o.t = tText
  o.id = id
  o.name = name
  o.text = text or ""
  o.action = function() end
  return o
end

function Text:get()
  return self.text
end

function Text:set(v, silent)
  local silent = silent or false
  if self.text ~= v then
    self.text = v
    if silent==false then self:bang() end
  end
end

function Text:delta(d)
  --noop
end

function Text:set_default()
  --noop
end

function Text:bang()
  self.action(self.text)
end

function Text:string()
  -- any formatting here? concat?
  return self.text
end


return Text
