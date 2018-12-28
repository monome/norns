--- Music utility module.
-- Utility methods for working with notes and scales.
--
-- @module MusicUtil
-- @release v1.0.2
-- @author Mark Eats

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


local function lookup_scale(scale_type)
  
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
  
  return MusicUtil.SCALES[scale_type]
end

local function generate_scale_array(root_num, scale_data, length)
  local out_array = {}
  local scale_len = #scale_data.intervals
  local note_num
  local i = 0
  while #out_array < length do
    if i > 0 and i % scale_len == 0 then
      root_num = root_num + scale_data.intervals[scale_len]
    else
      note_num = root_num + scale_data.intervals[i % scale_len + 1]
      if note_num > 127 then break
      else table.insert(out_array, note_num) end
    end
    i = i + 1
  end
  return out_array
end

--- Generate scale from a root note.
-- @param root_num MIDI note number (0-127) where scale will begin.
-- @param scale_type String defining scale type (eg, "major", "aeolian" or "neapolitan major"), see class for full list.
-- @param[opt] octaves Number of octaves to return, defaults to 1.
-- @return Array of MIDI note numbers.
function MusicUtil.generate_scale(root_num, scale_type, octaves)
  if type(root_num) ~= "number" or root_num < 0 or root_num > 127 then return nil end
  scale_type = scale_type or 1
  octaves = octaves or 1
  
  local scale_data = lookup_scale(scale_type)
  if not scale_data then return nil end
  local length = octaves * #scale_data.intervals - (util.round(octaves) - 1)
  
  return generate_scale_array(root_num, scale_data, length)
end

--- Generate given number of notes of a scale from a root note.
-- @param root_num MIDI note number (0-127) where scale will begin.
-- @param scale_type String defining scale type (eg, "major", "aeolian" or "neapolitan major"), see class for full list.
-- @param length Number of notes to return, defaults to 8.
-- @return Array of MIDI note numbers.
function MusicUtil.generate_scale_of_length(root_num, scale_type, length)
  length = length or 8
  
  local scale_data = lookup_scale(scale_type)
  if not scale_data then return nil end
  
  return generate_scale_array(root_num, scale_data, length)
end


--- Snap a MIDI note number to the nearest note number in an array.
-- @param note_num MIDI note number input (0-127).
-- @param snap_array Array of MIDI note numbers to snap to, must be in low to high order.
-- @return Adjusted note number.
function MusicUtil.snap_note_to_array(note_num, snap_array)
  local snap_array_len = #snap_array
  if snap_array_len == 1 then
    note_num = snap_array[1]
  elseif note_num >= snap_array[snap_array_len] then
    note_num = snap_array[snap_array_len]
  else
    local delta
    local prev_delta = math.huge
    for s = 1, snap_array_len + 1 do
      if s > snap_array_len then
        note_num = note_num + prev_delta
        break
      end
      delta = snap_array[s] - note_num
      if delta == 0 then
        break
      elseif math.abs(delta) >= math.abs(prev_delta) then
        note_num = note_num + prev_delta
        break
      end
      prev_delta = delta
    end
  end

  return note_num
end

--- Snap an array of MIDI note numbers to an array of note numbers.
-- @param note_nums_array Array of input MIDI note numbers.
-- @param snap_array Array of MIDI note numbers to snap to, must be in low to high order.
-- @return Array of adjusted note numbers.
function MusicUtil.snap_notes_to_array(note_nums_array, snap_array)
  for i = 1, #note_nums_array do
    note_nums_array[i] = MusicUtil.snap_note_to_array(note_nums_array[i], snap_array)
  end
  return note_nums_array
end


--- Return a MIDI note number's note name.
-- @param note_num MIDI note number (0-127).
-- @param[opt] include_octave Include octave number in return string if set to true.
-- @return Name string (eg, "C#3").
function MusicUtil.note_num_to_name(note_num, include_octave)
  local name = MusicUtil.NOTE_NAMES[note_num % 12 + 1]
  if include_octave then name = name .. math.floor(note_num / 12 - 2) end
  return name
end

--- Return an array of MIDI note numbers' names.
-- @param note_nums_array Array of MIDI note numbers.
-- @param[opt] include_octave Include octave number in return strings if set to true.
-- @return Array of name strings.
function MusicUtil.note_nums_to_names(note_nums_array, include_octave)
  local out_array = {}
  for i = 1, #note_nums_array do
    out_array[i] = MusicUtil.note_num_to_name(note_nums_array[i], include_octave)
  end
  return out_array
end


--- Return a MIDI note number's frequency.
-- @param note_num MIDI note number (0-127).
-- @return Frequency number in Hz.
function MusicUtil.note_num_to_freq(note_num)
  return 13.75 * (2 ^ ((note_num - 9) / 12))
end

--- Return an array of MIDI note numbers' frequencies.
-- @param note_nums_array Array of MIDI note numbers.
-- @return Array of frequency numbers in Hz.
function MusicUtil.note_nums_to_freqs(note_nums_array)
  local out_array = {}
  for i = 1, #note_nums_array do
    out_array[i] = MusicUtil.note_num_to_freq(note_nums_array[i])
  end
  return out_array
end


--- Return a frequency's nearest MIDI note number.
-- @param freq Frequency number in Hz.
-- @return MIDI note number (0-127).
function MusicUtil.freq_to_note_num(freq)
  return util.clamp(math.floor(12 * math.log(freq / 440.0) / math.log(2) + 69.5), 0, 127)
end

--- Return an array of frequencies' nearest MIDI note numbers.
-- @param freqs_array Array of frequency numbers in Hz.
-- @return Array of MIDI note numbers.
function MusicUtil.freqs_to_note_nums(freqs_array)
  local out_array = {}
  for i = 1, #freqs_array do
    out_array[i] = MusicUtil.freq_to_note_num(freqs_array[i])
  end
  return out_array
end


--- Return the ratio of an interval.
-- @param interval Interval in semitones.
-- @return Ratio number.
function MusicUtil.interval_to_ratio(interval)
  return math.pow(2, interval / 12)
end

--- Return an array of ratios of intervals.
-- @param intervals_array Array of intervals in semitones.
-- @return Array of ratio numbers.
function MusicUtil.intervals_to_ratios(intervals_array)
  local out_array = {}
  for i = 1, #intervals_array do
    out_array[i] = MusicUtil.interval_to_ratio(intervals_array[i])
  end
  return out_array
end


--- Return the interval of a ratio.
-- @param ratio Ratio number.
-- @return Interval in semitones.
function MusicUtil.ratio_to_interval(ratio)
  return 12 * math.log(ratio) / math.log(2)
end

--- Return an array of intervals of ratios.
-- @param ratios_array Array of ratio numbers.
-- @return Array of intervals in semitones.
function MusicUtil.ratios_to_intervals(ratios_array)
  local out_array = {}
  for i = 1, #ratios_array do
    out_array[i] = MusicUtil.ratio_to_interval(ratios_array[i])
  end
  return out_array
end


return MusicUtil
