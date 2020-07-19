--
-- Behavior
--

local types = sky.types

local Behavior = sky.Device:extend()

function Behavior:new(props)
  Behavior.super.new(self, props)
end

function Behavior:process(event, output)
  if self.bypass then
    output(event)
    return
  end

  local t = event.type
  local ch = event.ch

  -- TODO: rebuild this as a table??

  if t == types.NOTE_ON and self.note_on then
    self.note_on(event.note, event.vel, ch)
  elseif t == types.NOTE_OFF and self.note_off then
    self.note_off(event.note, event.vel, ch)
  elseif t == types.PITCH_BEND and self.pitch_bend then
    self.pitch_bend(event.val, ch)
  elseif t == types.CHANNEL_PRESSURE and self.channel_pressure then
    self.channel_pressure(event.val, ch)
  elseif t == types.KEY_PRESSURE and self.key_pressure then
    self.key_pressure(event.val, ch)
  elseif t == types.CONTROL_CHANGE and self.control_change then
    self.control_change(event.cc, event.val, ch)
  elseif t == types.CLOCK and self.clock then
    self.clock()
  elseif t == types.START and self.start then
    self.start()
  elseif t == types.STOP and self.stop then
    self.stop()
  elseif t == types.CONTINUE and self.continue then
    self.continue()
  elseif t == types.PROGRAM_CHANGE and self.program_change then
    self.program_change(event.val, ch)
  end

  output(event)
end

return {
  Behavior = Behavior,
}