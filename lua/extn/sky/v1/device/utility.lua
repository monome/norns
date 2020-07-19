
--
-- Func class
--

local Func = sky.Device:extend()

function Func:new(f)
  Func.super.new(self)
  self.f = f
end

function Func:process(event, output, state)
  if self.bypass then
    output(event)
  else
    self.f(event, output, state)
  end
end


--
-- Thru class
--

local Thru = sky.Device:extend()

function Thru:new(props)
  Thru.super.new(self, props)
end

--
-- Logger
--

local tu = require('tabutil')

local Logger = sky.Device:extend()

function Logger:new(props)
  Logger.super.new(self, props)
  self.show_beats = props.show_beats or false
  self.filter = props.filter or function(...) return false end
end

function Logger:process(event, output, state)
  if not self.bypass then
    local c = state.process_count
    if not self.filter(event) then
      if self.show_beats then
        print(c, clock.get_beats(), sky.to_string(event))
      else
        print(c, sky.to_string(event))
      end
    end
  end
  -- always output incoming event
  output(event)
end

--
-- Map
--

local Map = sky.Device:extend()

function Map:new(props)
  Map.super.new(self, props)
  self.match = props.match or function(e) return false end
  self.action = props.action or function(e) return e end
end

function Map:process(event, output, state)
  if self.match(event) then
    output(self.action(event))
  else
    output(event)
  end
end

--
-- module
--
return {
  Func = Func,
  Thru = Thru,
  Logger = Logger,
  Map = Map,
}
