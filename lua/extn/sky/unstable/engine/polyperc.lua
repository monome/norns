--
-- PolyPerc (glue)
--

-- mutate global used by matron to select engine
engine.name = "PolyPerc"

local Singleton = nil

local PolyPerc = sky.Device:extend()

function PolyPerc:new(props)
  PolyPerc.super.new(self, props)
  self:add_params()
end

function PolyPerc:process(event, output, state)
  if event.type == sky.types.NOTE_ON then
    engine.amp(util.linlin(0, 127, 0.05, 1, event.vel))
    engine.hz(sky.to_hz(event.note))
  end
  output(event)
end

function PolyPerc:add_params()
  params:add_separator('polyperc')
  params:add{ type = 'control', id = 'amp',
    controlspec = controlspec.new(0, 1, 'lin', 0, 0.5, ''),
    action = function(x) engine.amp(x) end
  }
  params:add{ type = 'control', id='pw',
    controlspec = controlspec.new(0, 100, 'lin', 0, 50, '%'),
    action = function(x) engine.pw(x / 100) end
  }
  params:add{ type = 'control', id = 'release',
    controlspec = controlspec.new(0.1, 3.2, 'lin', 0, 1.2, 's'),
    action = function(x) engine.release(x) end
  }
  params:add{ type = 'control', id = 'cutoff',
    controlspec = controlspec.new(50, 5000, 'exp', 0, 800, 'hz'),
    action = function(x) engine.cutoff(x) end
  }
  params:add{ type = 'control', id = 'gain',
    controlspec = controlspec.new(0, 4, 'lin', 0, 1, ''),
    action = function(x) engine.gain(x) end
  }
end

local function shared_instance(props)
  if Singleton == nil then
    Singleton = PolyPerc(props)
  end
  return Singleton
end

return {
  PolyPerc = shared_instance,
}