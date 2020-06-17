--
-- Switcher class
--
local Switcher = sky.Device:extend()

function Switcher:new(props)
  Switcher.super.new(self, props)
  self.which = props.which or 1
  for i, child in ipairs(props) do
    self[i] = child
  end
end

function Switcher:process(event, output, state)
  local chain = self[self.which]
  if chain ~= nil then
    chain:process(event, output, state)
  end
end

return {
  Switcher = Switcher,
}