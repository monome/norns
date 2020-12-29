--- module for creating a multitrack sequencer based on a single fast "superclock"
--
-- @module Lattice
-- @release v1.0.0
-- @author tyleretters & ezra

local Lattice, Track = {}, {}

--- instantiate a new lattice
-- @tparam[opt] number meter number of quarter notes per measure, defaults to 4
-- @tparam[opt] number ppqn the number of pulses per quarter note of this superclock, defaults to 96
-- @treturn table a new lattice
function Lattice:new(meter, ppqn)
  local s = setmetatable({}, { __index = Lattice })
  s.meter = meter ~= nil and meter or 4
  s.ppqn = ppqn ~= nil and ppqn or 96
  s.downbeat = nil -- downbeat callback
  s.transport = 0
  s.is_playing = false
  s.superclock_id = nil
  s.tracks = {}
  return s
end

--- start running the sequencer
function Lattice:start()
  if not self.superclock_id then
    self.superclock_id = clock.run(self.pulse, self)
  end
  self.is_playing = true
end

--- stop the sequencer
function Lattice:stop()
  self.is_playing = false
end

--- toggle the lattice
function Lattice:toggle()
  self.is_playing = not self.is_playing
end

--- destroy the lattice
function Lattice:destroy()
  self:stop()
  clock.cancel(self.superclock_id)
  self.tracks = {}
end

--- destroy a track lattice
-- @tparam table a track from this lattice
function Lattice:destroy_track(track)
  self.tracks[track.id] = nil
end

--- set the meter of the lattice
-- @tparam number meter the meter the lattice counts
function Lattice:set_meter(meter)
  self.meter = meter
end

--- set the ppqn of the lattice
-- @tparam number ppqn the pulses per quarter note
function Lattice:set_ppqn(ppqn)
  self.ppqn = ppqn
end

--- advance all tracks in this lattice a single by pulse
-- @tparam table s this lattice
function Lattice.pulse(s)
  while true do
    clock.sync(1/s.ppqn)
    if s.is_playing then
      s.transport = s.transport + 1
      if s.downbeat ~= nil and s.transport % (s.ppqn * s.meter) == 1 then
        s.downbeat(s.transport)
      end
      for id, track in pairs(s.tracks) do
        if track.is_playing then
          track.phase = track.phase + 1
          if track.phase > (track.division * s.ppqn * s.meter) then
            track.phase = track.phase - (track.division * s.ppqn * s.meter)
            track.event(track.phase)
          end
        end
      end
    end
  end
end 

--- factory method to add a new track to this lattice
-- @tparam number division the division of the track
-- @tparam function event callback event
-- @tparam[opt] boolean is the track playing?
-- @treturn table a new track
function Lattice:new_track(division, event, playing)
  local id = #self.tracks + 1
  local track = Track:new(id, division, event, playing)
  self.tracks[id] = track
  return track
end

--- instantiate a new track
-- @tparam number id numeric id for this track
-- @tparam number division the division of the track
-- @tparam function event callback event
-- @tparam[opt] boolean is the track playing?
-- @treturn table a new track
function Track:new(id, division, event, playing)
  local t = setmetatable({}, { __index = Track })
  t.id = id
  t.division = division
  t.event = event
  t.is_playing = (playing == nil) and true or playing
  t.phase = 0
  return t
end

--- start the track
function Track:start()
  self.is_playing = true
end

--- stop the track
function Track:stop()
  self.is_playing = false
end

--- toggle the track
function Track:toggle()
  self.is_playing = not self.is_playing
end

--- set the division of the track
-- @tparam number n the division of the track
function Track:set_division(n)
   self.division = n
end

return Lattice
