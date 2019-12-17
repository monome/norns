--- ControlSpec Class
-- @module controlspec
-- @alias ControlSpec

local LinearWarp = {}
function LinearWarp.map(spec, value)
  return util.linlin(0, 1, spec.minval, spec.maxval, value)
end

function LinearWarp.unmap(spec, value)
  return util.linlin(spec.minval, spec.maxval, 0, 1, value)
end

local ExponentialWarp = {}
function ExponentialWarp.map(spec, value)
  return util.linexp(0, 1, spec.minval, spec.maxval, value)
end

function ExponentialWarp.unmap(spec, value)
  return util.explin(spec.minval, spec.maxval, 0, 1, value)
end

local function ampdb(amp)
  return math.log10(amp) * 20.0
end

local function dbamp(db)
  return math.pow(10.0, db*0.05)
end

local DbFaderWarp = {}

function DbFaderWarp.map(spec, value)
  local minval = spec.minval
  local maxval = spec.maxval
  local range = dbamp(maxval) - dbamp(minval)
  if range >= 0 then
    return ampdb(value * value * range + dbamp(minval))
  else
    return ampdb((1 - (1-value) * (1-value)) * range + dbamp(minval))
  end
end

function DbFaderWarp.unmap(spec, value)
  local minval = spec.minval
  local maxval = spec.maxval
  if spec:range() >= 0 then
    return math.sqrt((dbamp(value) - dbamp(minval)) / (dbamp(maxval) - dbamp(minval)))
  else
    return 1 - math.sqrt(1 - ((dbamp(value) - dbamp(minval)) / (dbamp(maxval) - dbamp(minval))))
  end
end

local ControlSpec = {}
ControlSpec.__index = ControlSpec

--- constructor
-- @tparam number minval the minimum value of the output range
-- @tparam number maxval the max value of the output range
-- @tparam string warp ('exp', 'db', 'lin') a shaping option for incoming data (default is 'lin')
-- @tparam number step quantization value. output will be rounded to a multiple of step
-- @tparam number default a default value for when no value has been set
-- @tparam string units an indicator for the unit of measure the data represents
-- @treturn ControlSpec
function ControlSpec.new(minval, maxval, warp, step, default, units)
  local s = setmetatable({}, ControlSpec)
  s.minval = minval
  s.maxval = maxval
  if type(warp) == "string" then
    if warp == 'exp' then
      s.warp = ExponentialWarp
    elseif warp == 'db' then
      s.warp = DbFaderWarp
    else
      s.warp = LinearWarp
    end
  else
    s.warp = LinearWarp
  end
  s.step = step or 0
  s.default = default or minval
  s.units = units or ""
  return s
end

function ControlSpec:cliphi()
  return math.max(self.minval, self.maxval)
end

function ControlSpec:cliplo()
  return math.min(self.minval, self.maxval)
end

--- transform an incoming value between 0 and 1 through this ControlSpec
-- @tparam number value the incoming value
-- @treturn number the transformed value
function ControlSpec:map(value)
  local clamped = util.clamp(value, 0, 1)
  return util.round(self.warp.map(self, clamped), self.step)
end

--- untransform a transformed value into its original value
-- @tparam number value a previously transformed value
-- @treturn number the reverse-transformed value
function ControlSpec:unmap(value)
  local clamped = util.clamp(util.round(value, self.step), self:cliplo(), self:cliphi())
  return self.warp.unmap(self, clamped)
end

function ControlSpec:constrain(value)
  return util.round(util.clamp(value, self:cliplo(), self:cliphi()), self.step)
end

function ControlSpec:range()
  return self.maxval - self.minval
end

function ControlSpec:ratio()
  return self.maxval / self.minval
end

--- copy this ControlSpec into a new one
-- @treturn ControlSpec a new ControlSpec
function ControlSpec:copy()
  local s = setmetatable({}, ControlSpec)
  s.minval = self.minval
  s.maxval = self.maxval
  s.warp = self.warp
  s.step = self.step
  s.default = self.default
  s.units = self.units
  return s
end

--- print out the configuration of this ControlSpec
function ControlSpec:print()
  for k,v in pairs(self) do
    print("ControlSpec:")
    print('>> ', k, v)
  end
end

--- Presets
-- @section presets

--- converts values to a unipolar range
ControlSpec.UNIPOLAR = ControlSpec.new(0, 1, 'lin', 0, 0, "")
--- converts values to bipolar range
ControlSpec.BIPOLAR = ControlSpec.new(-1, 1, 'lin', 0, 0, "")
--- converts to a frequency range
ControlSpec.FREQ = ControlSpec.new(20, 20000, 'exp', 0, 440, "Hz")
--- converts to a low frequency range
ControlSpec.LOFREQ = ControlSpec.new(0.1, 100, 'exp', 0, 6, "Hz")
--- converts to a mid frequency range
ControlSpec.MIDFREQ = ControlSpec.new(25, 4200, 'exp', 0, 440, "Hz")
--- converts to an exponential frequency range from 0.1 to 20khz
ControlSpec.WIDEFREQ = ControlSpec.new(0.1, 20000, 'exp', 0, 440, "Hz")
--- maps into pi for controlling phase values
ControlSpec.PHASE = ControlSpec.new(0, math.pi, 'lin', 0, 0, "")
--- converts to an exponential RQ range
ControlSpec.RQ = ControlSpec.new(0.001, 2, 'exp', 0, 0.707, "")
--- converts to a MIDI range (default 64)
ControlSpec.MIDI = ControlSpec.new(0, 127, 'lin', 0, 64, "")
--- converts to a range for MIDI notes (default 60)
ControlSpec.MIDINOTE = ControlSpec.new(0, 127, 'lin', 0, 60, "")
--- converts to a range for MIDI velocity (default 64)
ControlSpec.MIDIVELOCITY = ControlSpec.new(1, 127, 'lin', 0, 64, "")
--- converts to a dB range
ControlSpec.DB = ControlSpec.new(-math.huge, 0, 'db', nil, nil, "dB")
--- converts to a linear amplitude range (0-1)
ControlSpec.AMP = ControlSpec.new(0, 1, 'lin', 0, 0, "") -- TODO: this uses \amp warp == FaderWarp in SuperCollider, would be good to have in lua too
--- converts to a boost/cut range in dB
ControlSpec.BOOSTCUT = ControlSpec.new(-20, 20, 'lin', 0, 0, "dB")
--- converts to a panning range (-1 - 1)
ControlSpec.PAN = ControlSpec.new(-1, 1, 'lin', 0, 0, "")
--- converts to a detune range
ControlSpec.DETUNE = ControlSpec.new(-20, 20, 'lin', 0, 0, "Hz")
--- converts to a sensible range for playback rate
ControlSpec.RATE = ControlSpec.new(0.125, 8, 'exp', 0, 1, "")
--- convert to a good range for beats
ControlSpec.BEATS = ControlSpec.new(0, 20, 'lin', 0, 0, "Hz")
--- converts to a good range for delay
ControlSpec.DELAY = ControlSpec.new(0.0001, 1, 'exp', 0, 0.3, "secs")

return ControlSpec
