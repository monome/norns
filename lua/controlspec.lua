local function map(warp, minval, maxval, value)
end

local function unmap(warp, minval, maxval, value)
end

local ControlSpec = {}
ControlSpec.WARP_LIN = 1
ControlSpec.WARP_EXP = 2
ControlSpec.__index = ControlSpec

function ControlSpec.new(minval, maxval, warp, step, default, units)
  local s = setmetatable({}, ControlSpec)
  s.minval = minval
  s.maxval = maxval
  if type(warp) == "string" then
    if warp == 'exp' then
      s.warp = ControlSpec.WARP_EXP
    else
      s.warp = ControlSpec.WARP_LIN
    end
  elseif type(warp) == "number" then
    s.warp = warp -- TODO: assumes number is in [ControlSpec.WARP_LIN, ControlSpec.WARP_EXP]
  else
    s.warp = ControlSpec.WARP_LIN
  end
  s.step = step
  s.default = default or minval -- TODO: test to ensure minval fallback works
  s.units = units or ""
  return s
end

function ControlSpec:map(value)
  if self.warp == ControlSpec.WARP_LIN then
    return util.linlin(0, 1, self.minval, self.maxval, value)
  elseif self.warp == ControlSpec.WARP_EXP then
    return util.linexp(0, 1, self.minval, self.maxval, value)
  end
end

function ControlSpec:unmap(value)
  if self.warp == ControlSpec.WARP_LIN then
    return util.linlin(self.minval, self.maxval, 0, 1, value)
  elseif self.warp == ControlSpec.WARP_EXP then
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

--[[
TODO 1:
consider defining these default specs as global constants, ie. ControlSpec.UNIPOLAR, ControlSpec.BIPOLAR, ControlSpec.FREQ, etc (akin to how SuperCollider works) however, since afaik there's no way of freezing objects in lua either storing default specs this way as globals is error prone: if someone changes the properties of a ControlSpec.GLOBAL spec it would affect all usages (this is the root cause of weird unexpected errors in SuperCollider too)
TODO 2: naming consider removing _spec suffix
]]
function ControlSpec.unipolar_spec()
  return ControlSpec.new(0, 1, 'lin', 0, 0, "")
end

function ControlSpec.bipolar_spec()
  return ControlSpec.new(-1, 1, 'lin', 0, 0, "")
end

function ControlSpec.freq_spec()
  return ControlSpec.new(20, 20000, 'exp', 0, 440, "Hz")
end

function ControlSpec.lofreq_spec()
  return ControlSpec.new(0.1, 100, 'exp', 0, 6, "Hz")
end

function ControlSpec.midfreq_spec()
  return ControlSpec.new(25, 4200, 'exp', 0, 440, "Hz")
end

function ControlSpec.widefreq_spec()
  return ControlSpec.new(0.1, 20000, 'exp', 0, 440, "Hz")
end

function ControlSpec.phase_spec()
  return ControlSpec.new(0, math.pi, 'lin', 0, 0, "")
end

function ControlSpec.rq_spec()
  return ControlSpec.new(0.001, 2, 'exp', 0, 0.707, "")
end

function ControlSpec.midi_spec()
  return ControlSpec.new(0, 127, 'lin', 0, 64, "")
end

function ControlSpec.midinote_spec()
  return ControlSpec.new(0, 127, 'lin', 0, 60, "")
end

function ControlSpec.midivelocity_spec()
  return ControlSpec.new(1, 127, 'lin', 0, 64, "")
end

function ControlSpec.db_spec()
  return ControlSpec.new(-60, 0, 'lin', nil, nil, "dB") -- TODO: this uses DbFaderWarp in SuperCollider, would be good to have in lua too
end

function ControlSpec.amp_spec()
  return ControlSpec.new(0, 1, 'lin', 0, 0, "") -- TODO: this uses FaderWarp in SuperCollider, would be good to have in lua too
end

function ControlSpec.boostcut_spec()
  return ControlSpec.new(-20, 20, 'lin', 0, 0, "dB")
end

function ControlSpec.pan_spec()
  return ControlSpec.new(-1, 1, 'lin', 0, 0, "")
end

function ControlSpec.detune_spec()
  return ControlSpec.new(-20, 20, 'lin', 0, 0, "Hz")
end

function ControlSpec.rate_spec()
  return ControlSpec.new(0.125, 8, 'exp', 0, 1, "")
end

function ControlSpec.beats_spec()
  return ControlSpec.new(0, 20, 'lin', 0, 0, "Hz")
end

function ControlSpec.delay_spec()
  return ControlSpec.new(0.0001, 1, 'exp', 0, 0.3, "Hz")
end

return ControlSpec
