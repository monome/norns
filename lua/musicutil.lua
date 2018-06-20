--- Music utility module
-- Utility methods for working with notes and scales.
-- @module MusicUtil

local MusicUtil = {}

MusicUtil.NOTE_NAMES = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"}
MusicUtil.SCALES = {
  {name = "Major", alt_names = {"Ionian"}, intervals = {0, 2, 4, 5, 7, 9, 11, 12}},
  {name = "Natural Minor", alt_names = {"Minor", "Aeolian"}, intervals = {0, 2, 3, 5, 7, 8, 10, 12}},
  {name = "Harmonic Minor", intervals = {0, 2, 3, 5, 7, 8, 11, 12}},
  {name = "Melodic Minor", intervals = {0, 2, 3, 5, 7, 9, 11, 12}},
  {name = "Dorian", intervals = {0, 2, 3, 5, 7, 9, 10, 12}},
  {name = "Phrygian", intervals = {0, 1, 3, 5, 7, 8, 10, 12}},
  {name = "Lydian", intervals = {0, 2, 4, 6, 7, 9, 11, 12}},
  {name = "Mixolydian", intervals = {0, 2, 4, 5, 7, 9, 10, 12}},
  {name = "Locrian", intervals = {0, 1, 3, 5, 6, 8, 10, 12}},
  {name = "Gypsy Minor", intervals = {0, 2, 3, 6, 7, 8, 11, 12}},
  {name = "Whole Tone", intervals = {0, 2, 4, 6, 8, 10, 12}},
  {name = "Major Pentatonic", intervals = {0, 2, 4, 7, 9, 12}},
  {name = "Minor Pentatonic", intervals = {0, 3, 5, 7, 10, 12}},
  {name = "Major Bebop", intervals = {0, 2, 4, 5, 7, 8, 9, 11, 12}},
  {name = "Altered Scale", intervals = {0, 1, 3, 4, 6, 8, 10, 12}},
  {name = "Dorian Bebop", intervals = {0, 2, 3, 4, 5, 7, 9, 10, 12}},
  {name = "Mixolydian Bebop", intervals = {0, 2, 4, 5, 7, 9, 10, 11, 12}},
  {name = "Blues Scale", alt_names = {"Blues"}, intervals = {0, 3, 5, 6, 7, 10, 12}},
  {name = "Diminished Whole Half", intervals = {0, 2, 3, 5, 6, 8, 9, 11, 12}},
  {name = "Diminished Half Whole", intervals = {0, 1, 3, 4, 6, 7, 9, 10, 12}},
  {name = "Neapolitan Major", intervals = {0, 1, 3, 5, 7, 9, 11, 12}},
  {name = "Hungarian Major", intervals = {0, 3, 4, 6, 7, 9, 10, 12}},
  {name = "Harmonic Major", intervals = {0, 2, 4, 5, 7, 8, 11, 12}},
  {name = "Hungarian Minor", intervals = {0, 2, 3, 6, 7, 8, 11, 12}},
  {name = "Lydian Minor", intervals = {0, 2, 4, 6, 7, 8, 10, 12}},
  {name = "Neapolitan Minor", intervals = {0, 1, 3, 5, 7, 8, 11, 12}},
  {name = "Major Locrian", intervals = {0, 2, 4, 5, 6, 8, 10, 12}},
  {name = "Leading Whole Tone", intervals = {0, 2, 4, 6, 8, 10, 11, 12}},
  {name = "Six Tone Symmetrical", intervals = {0, 1, 4, 5, 8, 9, 11, 12}},
  {name = "Arabian", intervals = {0, 2, 4, 5, 6, 8, 10, 12}},
  {name = "Balinese", intervals = {0, 1, 3, 7, 8, 12}},
  {name = "Byzantine", intervals = {0, 1, 3, 5, 7, 8, 11, 12}},
  {name = "Hungarian Gypsy", intervals = {0, 2, 4, 6, 7, 8, 10, 12}},
  {name = "Persian", intervals = {0, 1, 4, 5, 6, 8, 11, 12}},
  {name = "East Indian Purvi", intervals = {0, 1, 4, 6, 7, 8, 11, 12}},
  {name = "Oriental", intervals = {0, 1, 4, 5, 6, 9, 10, 12}},
  {name = "Double Harmonic", intervals = {0, 1, 4, 5, 7, 8, 11, 12}},
  {name = "Enigmatic", intervals = {0, 1, 4, 6, 8, 10, 11, 12}},
  {name = "Overtone", intervals = {0, 2, 4, 6, 7, 9, 10, 12}},
  {name = "Eight Tone Spanish", intervals = {0, 1, 3, 4, 5, 6, 8, 10, 12}},
  {name = "Prometheus", intervals = {0, 2, 4, 6, 9, 10, 12}},
  {name = "Gagaku Rittsu Sen Pou", intervals = {0, 2, 5, 7, 9, 10, 12}},
  {name = "Gagaku Ryo Sen Pou", intervals = {0, 2, 4, 7, 9, 12}},
  {name = "Zokugaku Yo Sen Pou", intervals = {0, 3, 5, 7, 10, 12}},
  {name = "In Sen Pou", intervals = {0, 1, 5, 2, 8, 12}},
  {name = "Okinawa", intervals = {0, 4, 5, 7, 11, 12}},
  {name = "Chromatic", intervals = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}}
}
-- Scale data from https://github.com/fredericcormier/WesternMusicElements


--- Generate scale from a root note.
-- @param root_num MIDI note number (0-127) where scale will begin
-- @param scale_type String defining scale type (eg, "major", "aeolian" or "neapolitan major"), see class for full list
-- @param octaves Number of octaves to return, defaults to 1
-- @return Array of MIDI note numbers
function MusicUtil.generate_scale(root_num, scale_type, octaves)
  if root_num < 0 or root_num > 127 then return nil end
  octaves = octaves or 1
  scale_type = scale_type or 1

  -- Lookup by name
  if type(scale_type) == "string" then
    scale_type = string.lower(scale_type)
    for i = 1, #MusicUtil.SCALES do
      if string.lower(MusicUtil.SCALES[i].name) == scale_type then
        scale_type = i
        break
      elseif MusicUtil.SCALES[i].alt_names then
        local found = false
        for j = 1, #MusicUtil.SCALES[i].alt_names do
          if string.lower(MusicUtil.SCALES[i].alt_names[j]) == scale_type then
            scale_type = i
            found = true
            break
          end
        end
        if found then break end
      end
    end
  end
  
  local scale_data =  MusicUtil.SCALES[scale_type]
  if not scale_data then return nil end

  -- Generate output array
  local output = {}
  local scale_length = #scale_data.intervals
  local note_num
  for i = 0, octaves * scale_length - 1 do
    if i > 0 and i % scale_length == 0 then
      root_num = root_num + scale_data.intervals[scale_length]
    else
      note_num = root_num + scale_data.intervals[i % scale_length + 1]
      if note_num > 127 then break
      else table.insert(output, note_num) end
    end
  end
  return output
end

--- Snap MIDI note numbers to an array of note numbers.
-- @param note_nums MIDI note number (0-127) or array of note numbers
-- @param snap_array Array of note numbers to snap to, must be in low to high order
-- return Array of adjusted note numbers or a single note number
function MusicUtil.snap_notes_to_array(note_nums, snap_array)
  if not note_nums or not snap_array then return nil end
  if type(note_nums) == "number" then note_nums = {note_nums} end
  
  local delta
  local prev_delta
  for n = 1, #note_nums do
    prev_delta = math.huge
    if note_nums[n] >= snap_array[#snap_array] then
      note_nums[n] = snap_array[#snap_array]
    else
      for s = 1, #snap_array do
        delta = snap_array[s] - note_nums[n]
        if delta == 0 then
          break
        elseif math.abs(delta) >= math.abs(prev_delta) then
          note_nums[n] = note_nums[n] + prev_delta
          break
        end
        prev_delta = delta
      end
    end
  end
  if #note_nums == 1 then return note_nums[1]
  else return note_nums end
end

--- Convert MIDI note numbers to names.
-- @param note_nums MIDI note number (0-127) or array of note numbers
-- @param include_octave Optional, include octave number in return string if set to true
-- @return Name string (eg, "C#3") or array of strings
function MusicUtil.note_nums_to_names(note_nums, include_octave)
  if not note_nums then return nil end
  if type(note_nums) == "number" then note_nums = {note_nums} end
  local output = {}
  for i = 1, #note_nums do
    local name = MusicUtil.NOTE_NAMES[note_nums[i] % 12 + 1]
    if include_octave then name = name .. math.floor(note_nums[i] / 12 - 1) end
    output[i] = name
  end
  if #output == 1 then return output[1]
  else return output end
end

--- Convert MIDI note numbers to frequencies.
-- @param note_nums MIDI note number (0-127) or array of note numbers
-- @return Frequency number in Hz or array of frequencies
function MusicUtil.note_nums_to_freqs(note_nums)
  if not note_nums then return nil end
  if type(note_nums) == "number" then note_nums = {note_nums} end
  local output = {}
  for i = 1, #note_nums do
    output[i] = (440 / 32) * (2 ^ ((note_nums[i] - 9) / 12))
  end
  if #output == 1 then return output[1]
  else return output end
end

--- Convert frequencies to nearest MIDI note numbers.
-- @param freqs Frequency number in Hz or array of frequencies
-- @return MIDI note number (0-127) or array of note numbers
function MusicUtil.freqs_to_note_nums(freqs)
  if not freqs then return nil end
  if type(freqs) == "number" then freqs = {freqs} end
  local output = {}
  for i = 1, #freqs do
    output[i] = util.clamp(math.floor(12 * math.log(freqs[i] / 440.0) / math.log(2) + 69.5), 0, 127)
  end
  if #output == 1 then return output[1]
  else return output end
end

return MusicUtil