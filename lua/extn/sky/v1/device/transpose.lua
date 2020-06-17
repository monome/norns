--
-- Transpose (midi semitones)
--

local Transpose = sky.Device:extend()

function Transpose:new(props)
  Transpose.super.new(self, props)
  self.semitones = props.semitones or 0
end

function Transpose:process(event, output)
  if not self.bypass then
    if sky.is_note(event) then
      -- FIXME: need to dup?
      event.note = util.clamp(event.note + self.semitones, 0, 127)
    end
  end
  output(event)
end

return {
  Transpose = Transpose,
}