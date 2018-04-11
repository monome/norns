local WARP_LIN = 1
local WARP_EXP = 2

local ControlSpec = {}
ControlSpec.__index = ControlSpec

function ControlSpec.new(minval, maxval, warp, step, default, units)
  local s = setmetatable({}, ControlSpec)
  s.minval = minval
  s.maxval = maxval
  if type(warp) == "string" then
    if warp == 'exp' then
      s.warp = WARP_EXP
    else
      s.warp = WARP_LIN
    end
  elseif type(warp) == "number" then
    s.warp = warp -- TODO: assumes number is in [WARP_LIN, WARP_EXP]
  else
    s.warp = WARP_LIN
  end
  s.step = step
  s.default = default or minval -- TODO: test to ensure minval fallback works
  s.units = units or ""
  return s
end

function ControlSpec:map(value)
  if self.warp == WARP_LIN then
    return util.linlin(0, 1, self.minval, self.maxval, value)
  elseif self.warp == WARP_EXP then
    return util.linexp(0, 1, self.minval, self.maxval, value)
  end
end

function ControlSpec:unmap(value)
  if self.warp == WARP_LIN then
    return util.linlin(self.minval, self.maxval, 0, 1, value)
  elseif self.warp == WARP_EXP then
    return util.explin(self.minval, self.maxval, 0, 1, value)
  end
end

function ControlSpec:constrain(value)
  return util.round(util.clamp(value, self.minval, self.maxval), self.step or 0)
end

function ControlSpec:print()
  for k,v in pairs(self) do
    print("ControlSpec:")
    print('>> ', k, v)
  end
end

function ControlSpec.unipolar()
  return ControlSpec.new(0, 1, 'lin', 0, 0, "")
end

function ControlSpec.bipolar()
  return ControlSpec.new(-1, 1, 'lin', 0, 0, "")
end

function ControlSpec.freq()
  return ControlSpec.new(20, 20000, 'exp', 0, 440, "Hz")
end

function ControlSpec.lofreq()
  return ControlSpec.new(0.1, 100, 'exp', 0, 6, "Hz")
end

function ControlSpec.midfreq()
  return ControlSpec.new(25, 4200, 'exp', 0, 440, "Hz")
end

function ControlSpec.widefreq()
  return ControlSpec.new(0.1, 20000, 'exp', 0, 440, "Hz")
end

function ControlSpec.phase()
  return ControlSpec.new(0, math.pi, 'lin', 0, 0, "")
end

function ControlSpec.rq()
  return ControlSpec.new(0.001, 2, 'exp', 0, 0.707, "")
end

function ControlSpec.midi()
  return ControlSpec.new(0, 127, 'lin', 0, 64, "")
end

function ControlSpec.midinote()
  return ControlSpec.new(0, 127, 'lin', 0, 60, "")
end

function ControlSpec.midivelocity()
  return ControlSpec.new(1, 127, 'lin', 0, 64, "")
end

function ControlSpec.db()
  return ControlSpec.new(-60, 0, 'lin', nil, nil, "dB") -- TODO: this uses DbFaderWarp in SuperCollider, would be good to have in lua too
end

function ControlSpec.amp()
  return ControlSpec.new(0, 1, 'lin', 0, 0, "") -- TODO: this uses FaderWarp in SuperCollider, would be good to have in lua too
end

function ControlSpec.boostcut()
  return ControlSpec.new(-20, 20, 'lin', 0, 0, "dB")
end

function ControlSpec.pan()
  return ControlSpec.new(-1, 1, 'lin', 0, 0, "")
end

function ControlSpec.detune()
  return ControlSpec.new(-20, 20, 'lin', 0, 0, "Hz")
end

function ControlSpec.rate()
  return ControlSpec.new(0.125, 8, 'exp', 0, 1, "")
end

function ControlSpec.beats()
  return ControlSpec.new(0, 20, 'lin', 0, 0, "Hz")
end

function ControlSpec.delay()
  return ControlSpec.new(0.0001, 1, 'exp', 0, 0.3, "Hz")
end

return ControlSpec
