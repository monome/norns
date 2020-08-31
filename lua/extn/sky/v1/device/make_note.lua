local DefaultTable = require('container/defaulttable')

local MakeNote = sky.Device:extend()

function MakeNote:new(props)
  MakeNote.super.new(self, props)
  self.duration = props.duration or 1/16
  self._scheduler = nil
  self._active = DefaultTable.new(0)
end

function MakeNote:device_inserted(chain)
  if self._scheduler ~= nil then
    error('MakeNote: one instance cannot be used in multiple chains at the same time')
  end
  self._scheduler = chain:scheduler(self)
end

function MakeNote:device_removed(chain)
  self._scheduler = nil
end

function MakeNote:process(event, output, state)
  -- just output it if we previously scheduled it
  if event.from == self then
    local id = sky.note_id(event)
    self._active[id] = self._active[id] - 1
    output(event)
    return
  end

  -- filter out any note offs if we are going to apply a duration
  if sky.is_type(event, sky.types.NOTE_OFF) and self.duration then
    return
  end

  -- set duration if need be and schedule
  if sky.is_type(event, sky.types.NOTE_ON) then
    local id = sky.note_id(event)
    local instance = self._active[id] + 1
    self._active[id] = instance
    local cid = sky.note_cid(event, instance)
    event.correlation = cid
    -- stamp duration if there isn't one
    if event.duration == nil then
      event.duration = clock.get_beat_sec(self.duration)
    end

    output(event)

    if event.duration ~= nil then
      local note_off = sky.mk_note_off(event.note, 0, event.ch, cid)
      note_off.from = self
      note_off.voice = event.voice
      self._scheduler:sleep(event.duration, note_off)
    end
    return
  end

  -- something else or a NOTE_OFF because we aren't setting duration
  output(event)
end

return {
  MakeNote = MakeNote,
}