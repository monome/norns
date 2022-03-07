-- polysub

local cs = require 'controlspec'

local polysub = {}

function polysub.params()
  -- synth
  params:add{type = "control", id = "shape", name = "shape",
    controlspec = cs.new(0, 1, "lin", 0, 0, ""), action = engine.shape}
  params:add{type = "control", id = "timbre", name = "timbre",
    controlspec = cs.new(0, 1, "lin", 0, 0.5, ""), action = engine.timbre}
  params:add{type = "control", id = "sub", name = "sub",
    controlspec = cs.new(0, 1, "lin", 0, 0, ""), action = engine.sub}
  params:add{type = "control", id = "noise", name = "noise",
    controlspec = cs.new(0, 1, "lin", 0, 0, ""), action = engine.noise}
  params:add{type = "control", id = "detune", name = "detune",
    controlspec = cs.new(0, 1, "lin", 0, 0, ""), action = engine.detune}
  params:add{type = "control", id = "width", name = "stereo width",
    controlspec = cs.new(0, 1, "lin", 0, 0.5, ""), action = engine.width}
  params:add{type = "control", id = "hzlag", name = "pitch lag",
    controlspec = cs.new(0, 1, "lin", 0, 0, ""), action = engine.hzLag}

  -- filter
  params:add{type = "control", id = "fgain", name = "filter gain",
    controlspec = cs.new(0, 6, "lin", 0, 0, ""), action = engine.fgain}
  params:add{type = "control", id = "cut", name = "cut",
    controlspec = cs.new(0, 32, "lin", 0, 8, ""), action = engine.cut}
  params:add{type = "control", id = "cutenvamt", name = "cut env amt",
    controlspec = cs.new(0, 1, "lin", 0, 0, ""), action = engine.cutEnvAmt}
  params:add{type = "control", id = "cutatk", name = "cut attack",
    controlspec = cs.new(0.01, 10, "lin", 0, 0.05, ""), action = engine.cutAtk}
  params:add{type = "control", id = "cutdec", name = "cut decay",
    controlspec = cs.new(0, 2, "lin", 0, 0.1, ""), action = engine.cutDec}
  params:add{type = "control", id = "cutsus", name = "cut sustain",
    controlspec = cs.new(0, 1, "lin", 0, 0.9, ""), action = engine.cutSus}
  params:add{type = "control", id = "cutrel", name = "cut release",
    controlspec = cs.new(0.01, 10, "lin", 0, 1, ""), action = engine.cutRel}

  -- amp
  params:add{type = "control", id = "level", name = "level",
    controlspec = cs.new(0, 1, "lin", 0, 0.15, ""), action = engine.level}
  params:add{type = "control", id = "ampatk", name = "amp attack",
    controlspec = cs.new(0.01, 10, "lin", 0, 0.05, ""), action = engine.ampAtk}
  params:add{type = "control", id = "ampdec", name = "amp decay",
    controlspec = cs.new(0, 2, "lin", 0, 0.1, ""), action = engine.ampDec}
  params:add{type = "control", id = "ampsus", name = "amp sustain",
    controlspec = cs.new(0, 1, "lin", 0, 0.9, ""), action = engine.ampSus}
  params:add{type = "control", id = "amprel", name = "amp release",
    controlspec = cs.new(0.01, 10, "lin", 0, 1, ""), action = engine.ampRel}
end

return polysub
