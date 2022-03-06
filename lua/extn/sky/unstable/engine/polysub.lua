--
-- PolySub (glue)
--

-- mutate global used by matron to select engine
engine.name = "PolySub"

local glue = require('polysub')

local Singleton = nil

local PolySub = sky.Device:extend()

function PolySub:new(props)
  PolySub.super.new(self, props)
  self.next_voice = 1
  self.voices = {}
end

function PolySub:process(event, output, state)
  if event.type == sky.types.NOTE_ON then
    local existing = self.voices[event.note]
    if existing then
      engine.stop(existing)
    end
    engine.start(self.next_voice, sky.to_hz(event.note))
    self.voices[event.note] = self.next_voice
    self.next_voice = self.next_voice + 1
  elseif event.type == sky.types.NOTE_OFF then
    local existing = self.voices[event.note]
    if existing then
      engine.stop(existing)
    end
    self.voices[event.note] = nil
  end
  output(event)
end

function PolySub:add_params(group)
  if group then
    -- MAINT: this must match the number of params defined by the polysub glue
    params:add_group('polysub', 19)
  else
    params:add_separator('polysub')
  end
  glue.params()
end

local function shared_instance(props)
  if Singleton == nil then
    Singleton = PolySub(props)
  end
  return Singleton
end

return {
  PolySub = shared_instance,
}
