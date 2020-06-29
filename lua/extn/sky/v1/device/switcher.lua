local util = require('util')

--
-- Switcher class
--
local Switcher = sky.Device:extend()

function Switcher:new(props)
  Switcher.super.new(self, props)
  self._active = {}
  self._count = 0
  for i, child in ipairs(props) do
    self[i] = child
    self._count = i
  end
  self:set_which(props.which or 1)
end

function Switcher:process(event, output, state)
  local chain = self[self.which]
  if chain ~= nil then
    if sky.is_type(event, sky.types.NOTE_ON) then
      self._active[event.correlation] = self.which
    elseif sky.is_type(event, sky.types.NOTE_OFF) then
      local prior = self._active[event.correlation]
      if prior then
        chain = self[prior]
      end
    end

    chain:process(event, output, state)
  end
end

function Switcher:set_which(which)
  self.which = util.clamp(which, 1, self._count)
end

return {
  Switcher = Switcher,
}