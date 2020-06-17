--
-- Filter
--

local Filter = sky.Device:extend()

function Filter:new(props)
  Filter.super.new(self, props)
  self._match = {}
  self.types = prop.types or {}
  -- FIXME: __newindex isn't updating table, BROKEN move stuff to props
  self:_build_type_table(self.types)

  if type(props.invert) == 'boolean' then
    self.invert = props.invert
  else
    self.invert = false
  end
end

function Filter:__newindex(idx, val)
  if idx == "types" then
    -- build event class filter
    self:_build_type_table(val)
    rawset(self, idx, val)
  else
    rawset(self, idx, val)
  end
end

function Filter:_build_type_table(val)
  local t = {}
  -- FIXME: this is a goofy way to do set membership
  for _, v in ipairs(val) do
    t[v] = true
  end
  self._match.types = t
end

function Filter:process(event, output)
  if self.bypass then
    output(event)
    return
  end

  -- TODO: expand on this
  local type_match = self._match.types[event.type]
  if type_match and self.invert then
    output(event)
  end
end

return {
  Filter = Filter,
}

