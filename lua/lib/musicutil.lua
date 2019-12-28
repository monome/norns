--- Music utility module.
-- Utility methods for working with notes, scales and chords.
--
-- @module MusicUtil
-- @release v1.1.1
-- @author Mark Eats

local MusicUtil = {}

MusicUtil.NOTE_NAMES = {"C", "C♯", "D", "D♯", "E", "F", "F♯", "G", "G♯", "A", "A♯", "B"}
MusicUtil.SCALES = {
  {name = "Major", alt_names = {"Ionian"}, intervals = {0, 2, 4, 5, 7, 9, 11, 12}, chords = {{1, 2, 3, 4, 5, 6, 7, 14}, {14, 15, 17, 18, 19, 20, 21, 22, 23}, {14, 15, 17, 19}, {1, 2, 3, 4, 5}, {1, 2, 4, 8, 9, 10, 11, 14, 15}, {14, 15, 17, 19, 21, 22}, {24, 26}, {1, 2, 3, 4, 5, 6, 7, 14}}},
  {name = "Natural Minor", alt_names = {"Minor", "Aeolian"}, intervals = {0, 2, 3, 5, 7, 8, 10, 12}, chords = {{14, 15, 17, 19, 21, 22}, {24, 26}, {1, 2, 3, 4, 5, 6, 7, 14}, {14, 15, 17, 18, 19, 20, 21, 22, 23}, {14, 15, 17, 19}, {1, 2, 3, 4, 5}, {1, 2, 4, 8, 9, 10, 11, 14, 15}, {14, 15, 17, 19, 21, 22}}},
  {name = "Harmonic Minor", intervals = {0, 2, 3, 5, 7, 8, 11, 12}, chords = {{14, 16, 17}, {24, 25, 26}, {12}, {17, 18, 19, 20, 21, 24, 25, 26}, {1, 8, 12, 13, 14, 15}, {1, 2, 3, 16, 17, 18, 24, 25}, {12, 24, 25}, {14, 16, 17}}},
  {name = "Melodic Minor", intervals = {0, 2, 3, 5, 7, 9, 11, 12}, chords = {{14, 16, 17, 18, 20}, {14, 15, 17, 18, 19}, {12}, {1, 2, 4, 8, 9}, {1, 8, 9, 10, 12, 13, 14, 15}, {24, 26}, {12, 13, 24, 26}, {14, 16, 17, 18, 20}}},
  {name = "Dorian", intervals = {0, 2, 3, 5, 7, 9, 10, 12}, chords = {{14, 15, 17, 18, 19, 20, 21, 22, 23}, {14, 15, 17, 19}, {1, 2, 3, 4, 5}, {1, 2, 4, 8, 9, 10, 11, 14, 15}, {14, 15, 17, 19, 21, 22}, {24, 26}, {1, 2, 3, 4, 5, 6, 7, 14}, {14, 15, 17, 18, 19, 20, 21, 22, 23}}},
  {name = "Phrygian", intervals = {0, 1, 3, 5, 7, 8, 10, 12}, chords = {{14, 15, 17, 19}, {1, 2, 3, 4, 5}, {1, 2, 4, 8, 9, 10, 11, 14, 15}, {14, 15, 17, 19, 21, 22}, {24, 26}, {1, 2, 3, 4, 5, 6, 7, 14}, {14, 15, 17, 18, 19, 20, 21, 22, 23}, {14, 15, 17, 19}}},
  {name = "Lydian", intervals = {0, 2, 4, 6, 7, 9, 11, 12}, chords = {{1, 2, 3, 4, 5}, {1, 2, 4, 8, 9, 10, 11, 14, 15}, {14, 15, 17, 19, 21, 22}, {24, 26}, {1, 2, 3, 4, 5, 6, 7, 14}, {14, 15, 17, 18, 19, 20, 21, 22, 23}, {14, 15, 17, 19}, {1, 2, 3, 4, 5}}},
  {name = "Mixolydian", intervals = {0, 2, 4, 5, 7, 9, 10, 12}, chords = {{1, 2, 4, 8, 9, 10, 11, 14, 15}, {14, 15, 17, 19, 21, 22}, {24, 26}, {1, 2, 3, 4, 5, 6, 7, 14}, {14, 15, 17, 18, 19, 20, 21, 22, 23}, {14, 15, 17, 19}, {1, 2, 3, 4, 5}, {1, 2, 4, 8, 9, 10, 11, 14, 15}}},
  {name = "Locrian", intervals = {0, 1, 3, 5, 6, 8, 10, 12}, chords = {{24, 26}, {1, 2, 3, 4, 5, 6, 7, 14}, {14, 15, 17, 18, 19, 20, 21, 22, 23}, {14, 15, 17, 19}, {1, 2, 3, 4, 5}, {1, 2, 4, 8, 9, 10, 11, 14, 15}, {14, 15, 17, 19, 21, 22}, {24, 26}}},
  {name = "Gypsy Minor", intervals = {0, 2, 3, 6, 7, 8, 11, 12}, chords = {{16, 17, 24}, {}, {12}, {}, {1, 3, 12, 14}, {1, 3, 8, 16, 17, 19, 24, 26}, {1, 2, 12, 17, 18}, {16, 17, 24}}},
  {name = "Whole Tone", intervals = {0, 2, 4, 6, 8, 10, 12}, chords = {{12, 13}, {12, 13}, {12, 13}, {12, 13}, {12, 13}, {12, 13}, {12, 13}}},
  {name = "Major Pentatonic", intervals = {0, 2, 4, 7, 9, 12}, chords = {{1, 2, 4}, {14, 15}, {}, {14}, {14, 15, 17, 19}, {1, 2, 4}}},
  {name = "Minor Pentatonic", intervals = {0, 3, 5, 7, 10, 12}, chords = {{14, 15, 17, 19}, {1, 2, 4}, {14, 15}, {}, {14}, {14, 15, 17, 19}}},
  {name = "Major Bebop", intervals = {0, 2, 4, 5, 7, 8, 9, 11, 12}, chords = {{1, 2, 3, 4, 5, 6, 7, 12, 14}, {14, 15, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26}, {1, 8, 12, 13, 14, 15, 17, 19}, {1, 2, 3, 4, 5, 16, 17, 18, 20, 24, 25}, {1, 2, 4, 8, 9, 10, 11, 14, 15}, {12, 24, 25}, {14, 15, 16, 17, 19, 21, 22}, {24, 25, 26}, {1, 2, 3, 4, 5, 6, 7, 12, 14}}},
  {name = "Altered Scale", intervals = {0, 1, 3, 4, 6, 8, 10, 12}, chords = {{12, 13, 24, 26}, {14, 16, 17, 18, 20}, {14, 15, 17, 18, 19}, {12}, {1, 2, 4, 8, 9}, {1, 8, 9, 10, 12, 13, 14, 15}, {24, 26}, {12, 13, 24, 26}}},
  {name = "Dorian Bebop", intervals = {0, 2, 3, 4, 5, 7, 9, 10, 12}, chords = {{1, 2, 4, 8, 9, 10, 11, 14, 15, 17, 18, 19, 20, 21, 22, 23}, {14, 15, 17, 19, 21, 22}, {1, 2, 3, 4, 5}, {24, 26}, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 14, 15}, {14, 15, 17, 18, 19, 20, 21, 22, 23}, {14, 15, 17, 19, 24, 26}, {1, 2, 3, 4, 5, 6, 7, 14}, {1, 2, 4, 8, 9, 10, 11, 14, 15, 17, 18, 19, 20, 21, 22, 23}}},
  {name = "Mixolydian Bebop", intervals = {0, 2, 4, 5, 7, 9, 10, 11, 12}, chords = {{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 14, 15}, {14, 15, 17, 18, 19, 20, 21, 22, 23}, {14, 15, 17, 19, 24, 26}, {1, 2, 3, 4, 5, 6, 7, 14}, {1, 2, 4, 8, 9, 10, 11, 14, 15, 17, 18, 19, 20, 21, 22, 23}, {14, 15, 17, 19, 21, 22}, {1, 2, 3, 4, 5}, {24, 26}, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 14, 15}}},
  {name = "Blues Scale", alt_names = {"Blues"}, intervals = {0, 3, 5, 6, 7, 10, 12}, chords = {{14, 15, 17, 19, 24, 26}, {1, 2, 4, 17, 18, 20}, {14, 15}, {}, {}, {14}, {14, 15, 17, 19, 24, 26}}},
  {name = "Diminished Whole Half", intervals = {0, 2, 3, 5, 6, 8, 9, 11, 12}, chords = {{24, 25}, {1, 2, 8, 17, 18, 19, 24, 25, 26}, {24, 25}, {1, 2, 8, 17, 18, 19, 24, 25, 26}, {24, 25}, {1, 2, 8, 17, 18, 19, 24, 25, 26}, {24, 25}, {1, 2, 8, 17, 18, 19, 24, 25, 26}, {24, 25}}},
  {name = "Diminished Half Whole", intervals = {0, 1, 3, 4, 6, 7, 9, 10, 12}, chords = {{1, 2, 8, 17, 18, 19, 24, 25, 26}, {24, 25}, {1, 2, 8, 17, 18, 19, 24, 25, 26}, {24, 25}, {1, 2, 8, 17, 18, 19, 24, 25, 26}, {24, 25}, {1, 2, 8, 17, 18, 19, 24, 25, 26}, {24, 25}, {1, 2, 8, 17, 18, 19, 24, 25, 26}}},
  {name = "Neapolitan Major", intervals = {0, 1, 3, 5, 7, 9, 11, 12}, chords = {{14, 16, 17, 18}, {12, 13}, {12, 13}, {1, 8, 9, 12, 13}, {12, 13}, {12, 13, 24, 26}, {12, 13}, {14, 16, 17, 18}}},
  {name = "Hungarian Major", intervals = {0, 3, 4, 6, 7, 9, 10, 12}, chords = {{1, 2, 8, 17, 18, 19, 24, 25, 26}, {1, 2, 17, 18, 24, 25}, {24}, {24, 25, 26}, {}, {17, 18, 19, 24, 25, 26}, {}, {1, 2, 8, 17, 18, 19, 24, 25, 26}}},
  {name = "Harmonic Major", intervals = {0, 2, 4, 5, 7, 8, 11, 12}, chords = {{1, 3, 5, 6, 12, 14}, {24, 25, 26}, {1, 8, 12, 13, 17, 19}, {16, 17, 18, 20, 24, 25}, {1, 2, 8, 14, 15}, {12, 24, 25}, {24, 25}, {1, 3, 5, 6, 12, 14}}},
  {name = "Hungarian Minor", intervals = {0, 2, 3, 6, 7, 8, 11, 12}, chords = {{16, 17, 24}, {}, {12}, {}, {1, 3, 12, 14}, {1, 3, 8, 16, 17, 19, 24, 26}, {1, 2, 12, 17, 18}, {16, 17, 24}}},
  {name = "Lydian Minor", intervals = {0, 2, 4, 6, 7, 8, 10, 12}, chords = {{1, 8, 9, 12, 13}, {12, 13}, {12, 13, 24, 26}, {12, 13}, {14, 16, 17, 18}, {12, 13}, {12, 13}, {1, 8, 9, 12, 13}}},
  {name = "Neapolitan Minor", intervals = {0, 1, 3, 5, 7, 8, 11, 12}, chords = {{14, 16, 17}, {1, 3, 5, 8, 9}, {12, 13}, {17, 19, 21, 24, 26}, {12, 13}, {1, 2, 3, 14, 16, 17, 18}, {12}, {14, 16, 17}}},
  {name = "Major Locrian", intervals = {0, 2, 4, 5, 6, 8, 10, 12}, chords = {{12, 13}, {12, 13, 24, 26}, {12, 13}, {14, 16, 17, 18}, {12, 13}, {12, 13}, {1, 8, 9, 12, 13}, {12, 13}}},
  {name = "Leading Whole Tone", intervals = {0, 2, 4, 6, 8, 10, 11, 12}, chords = {{12, 13}, {12, 13}, {1, 8, 9, 12, 13}, {12, 13}, {12, 13, 24, 26}, {12, 13}, {14, 16, 17, 18}, {12, 13}}},
  {name = "Six Tone Symmetrical", intervals = {0, 1, 4, 5, 8, 9, 11, 12}, chords = {{12}, {1, 3, 8, 12, 13, 16, 17, 19}, {1, 2, 12, 14}, {1, 3, 12, 16, 17, 24}, {12}, {1, 3, 5, 12, 16, 17}, {}, {12}}},
  {name = "Arabian", intervals = {0, 2, 4, 5, 6, 8, 10, 12}, chords = {{12, 13}, {12, 13, 24, 26}, {12, 13}, {14, 16, 17, 18}, {12, 13}, {12, 13}, {1, 8, 9, 12, 13}, {12, 13}}},
  {name = "Balinese", intervals = {0, 1, 3, 7, 8, 12}, chords = {{17}, {}, {}, {}, {1, 3, 14}, {17}}},
  {name = "Byzantine", intervals = {0, 1, 3, 5, 7, 8, 11, 12}, chords = {{14, 16, 17}, {1, 3, 5, 8, 9}, {12, 13}, {17, 19, 21, 24, 26}, {12, 13}, {1, 2, 3, 14, 16, 17, 18}, {12}, {14, 16, 17}}},
  {name = "Hungarian Gypsy", intervals = {0, 2, 4, 6, 7, 8, 10, 12}, chords = {{1, 8, 9, 12, 13}, {12, 13}, {12, 13, 24, 26}, {12, 13}, {14, 16, 17, 18}, {12, 13}, {12, 13}, {1, 8, 9, 12, 13}}},
  {name = "Persian", intervals = {0, 1, 4, 5, 6, 8, 11, 12}, chords = {{12}, {1, 3, 8, 14, 15, 16, 17, 19}, {1, 2, 4, 12}, {16, 17, 24}, {14, 15}, {12, 13}, {14}, {12}}},
  {name = "East Indian Purvi", intervals = {0, 1, 4, 6, 7, 8, 11, 12}, chords = {{1, 3, 12}, {14, 15, 16, 17, 19, 24, 26}, {1, 2, 4, 12, 17, 18, 20}, {14, 15}, {}, {12, 13}, {14}, {1, 3, 12}}},
  {name = "Oriental", intervals = {0, 1, 4, 5, 6, 9, 10, 12}, chords = {{}, {12}, {}, {1, 3, 12, 14}, {1, 3, 8, 16, 17, 19, 24, 26}, {1, 2, 12, 17, 18}, {16, 17, 24}, {}}},
  {name = "Double Harmonic", intervals = {0, 1, 4, 5, 7, 8, 11, 12}, chords = {{1, 3, 12, 14}, {1, 3, 8, 16, 17, 19, 24, 26}, {1, 2, 12, 17, 18}, {16, 17, 24}, {}, {12}, {}, {1, 3, 12, 14}}},
  {name = "Enigmatic", intervals = {0, 1, 4, 6, 8, 10, 11, 12}, chords = {{12, 13}, {14, 15, 16, 17, 18, 19}, {1, 2, 4, 12}, {1, 8, 9, 10, 14, 15}, {12, 13}, {24, 26}, {14}, {12, 13}}},
  {name = "Overtone", intervals = {0, 2, 4, 6, 7, 9, 10, 12}, chords = {{1, 2, 4, 8, 9}, {1, 8, 9, 10, 12, 13, 14, 15}, {24, 26}, {12, 13, 24, 26}, {14, 16, 17, 18, 20}, {14, 15, 17, 18, 19}, {12}, {1, 2, 4, 8, 9}}},
  {name = "Eight Tone Spanish", intervals = {0, 1, 3, 4, 5, 6, 8, 10, 12}, chords = {{12, 13, 24, 26}, {1, 2, 3, 4, 5, 6, 7, 14, 16, 17, 18, 20}, {14, 15, 17, 18, 19, 20, 21, 22, 23}, {12}, {14, 15, 16, 17, 19}, {1, 2, 3, 4, 5, 8, 9}, {1, 2, 4, 8, 9, 10, 11, 12, 13, 14, 15}, {14, 15, 17, 19, 21, 22, 24, 26}, {12, 13, 24, 26}}},
  {name = "Prometheus", intervals = {0, 2, 4, 6, 9, 10, 12}, chords = {{}, {1, 8, 9, 12, 13}, {}, {12, 13, 24, 26}, {14, 17, 18}, {12}, {}}},
  {name = "Gagaku Rittsu Sen Pou", intervals = {0, 2, 5, 7, 9, 10, 12}, chords = {{14, 15}, {14, 15, 17, 19}, {1, 2, 4, 14}, {14, 15, 17, 19, 21, 22}, {}, {1, 2, 3, 4, 5}, {14, 15}}},
  {name = "Gagaku Ryo Sen Pou", intervals = {0, 2, 4, 7, 9, 12}, chords = {{1, 2, 4}, {14, 15}, {}, {14}, {14, 15, 17, 19}, {1, 2, 4}}},
  {name = "Zokugaku Yo Sen Pou", intervals = {0, 3, 5, 7, 10, 12}, chords = {{14, 15, 17, 19}, {1, 2, 4}, {14, 15}, {}, {14}, {14, 15, 17, 19}}},
  {name = "In Sen Pou", intervals = {0, 1, 5, 2, 8, 12}, chords = {{}, {1, 3}, {17, 18}, {24, 26}, {}, {}}},
  {name = "Okinawa", intervals = {0, 4, 5, 7, 11, 12}, chords = {{1, 3, 14}, {17}, {}, {}, {}, {1, 3, 14}}},
  {name = "Chromatic", intervals = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}, chords = {{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26}, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26}, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26}, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26}, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26}, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26}, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26}, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26}, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26}, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26}, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26}, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26}, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26}}}
}
MusicUtil.CHORDS = {
  {name = "Major", alt_names = {"Maj"}, intervals = {0, 4, 7}},
  {name = "Major 6", alt_names = {"Maj6"}, intervals = {0, 4, 7, 9}},
  {name = "Major 7", alt_names = {"Maj7"}, intervals = {0, 4, 7, 11}},
  {name = "Major 69", alt_names = {"Maj69"}, intervals = {0, 4, 7, 9, 14}},
  {name = "Major 9", alt_names = {"Maj9"}, intervals = {0, 4, 7, 11, 14}},
  {name = "Major 11", alt_names = {"Maj11"}, intervals = {0, 4, 7, 11, 14, 17}},
  {name = "Major 13", alt_names = {"Maj13"}, intervals = {0, 4, 7, 11, 14, 17, 21}},
  {name = "Dominant 7", intervals = {0, 4, 7, 10}},
  {name = "Ninth", intervals = {0, 4, 7, 10, 14}},
  {name = "Eleventh", intervals = {0, 4, 7, 10, 14, 17}},
  {name = "Thirteenth", intervals = {0, 4, 7, 10, 14, 17, 21}},
  {name = "Augmented", intervals = {0, 4, 8}},
  {name = "Augmented 7", intervals = {0, 4, 8, 10}},
  {name = "Sus4", intervals = {0, 5, 7}},
  {name = "Seventh sus4", intervals = {0, 5, 7, 10}},
  {name = "Minor Major 7", alt_names = {"MinMaj7"}, intervals = {0, 3, 7, 11}},
  {name = "Minor", alt_names = {"Min"}, intervals = {0, 3, 7}},
  {name = "Minor 6", alt_names = {"Min6"}, intervals = {0, 3, 7, 9}},
  {name = "Minor 7", alt_names = {"Min7"}, intervals = {0, 3, 7, 10}},
  {name = "Minor 69", alt_names = {"Min69"}, intervals = {0, 3, 7, 9, 14}},
  {name = "Minor 9", alt_names = {"Min9"}, intervals = {0, 3, 7, 10, 14}},
  {name = "Minor 11", alt_names = {"Min11"}, intervals = {0, 3, 7, 10, 14, 17}},
  {name = "Minor 13", alt_names = {"Min13"}, intervals = {0, 3, 7, 10, 14, 17, 21}},
  {name = "Diminished", alt_names = {"Dim"}, intervals = {0, 3, 6}},
  {name = "Diminished 7", alt_names = {"Dim7"}, intervals = {0, 3, 6, 9}},
  {name = "Half Diminished 7", intervals = {0, 3, 6, 10}}
}
-- Data from https://github.com/fredericcormier/WesternMusicElements


-- Used offline to generate the chord cross-references in the SCALES table above
-- Needs to be updated when either SCALES or CHORDS changes
--[[
local function generate_chord_lookups()
  
  for s = 1, #MusicUtil.SCALES do
    MusicUtil.SCALES[s].chords = {}
    local num_scale_intervals = #MusicUtil.SCALES[s].intervals
    for si = 1, num_scale_intervals do
      MusicUtil.SCALES[s].chords[si] = {}
      for c = 1, #MusicUtil.CHORDS do
        local in_key = true
        for ci = 1, #MusicUtil.CHORDS[c].intervals do
          local chord_interval_in_key = false
          for sii = 1, num_scale_intervals do
            if (MusicUtil.CHORDS[c].intervals[ci] + MusicUtil.SCALES[s].intervals[si]) % 12 == MusicUtil.SCALES[s].intervals[sii] then
              chord_interval_in_key = true
              break
            end
          end
          if not chord_interval_in_key then
            in_key = false
            break
          end
        end
        if in_key then
          table.insert(MusicUtil.SCALES[s].chords[si], c)
        end
      end
    end

    -- Print it all
    local scale_string = ""
    scale_string = "{name = \"" .. MusicUtil.SCALES[s].name .. "\""

    if MusicUtil.SCALES[s].alt_names then
      scale_string = scale_string .. ", alt_names = {"
      for an = 1, #MusicUtil.SCALES[s].alt_names do
        scale_string = scale_string .. "\"" .. MusicUtil.SCALES[s].alt_names[an] .. "\""
        if an < #MusicUtil.SCALES[s].alt_names then
          scale_string = scale_string .. ", "
        end
      end
    scale_string = scale_string .. "}"
    end
    scale_string = scale_string .. ", intervals = {"
    for int = 1, #MusicUtil.SCALES[s].intervals do
      scale_string = scale_string .. MusicUtil.SCALES[s].intervals[int]
      if int < #MusicUtil.SCALES[s].intervals then
        scale_string = scale_string .. ", "
      end
    end
    scale_string = scale_string .. "}, chords = {"
    for c = 1, #MusicUtil.SCALES[s].chords do
      scale_string = scale_string .. "{"
      for ci = 1, #MusicUtil.SCALES[s].chords[c] do
        scale_string = scale_string .. MusicUtil.SCALES[s].chords[c][ci]
        if ci < #MusicUtil.SCALES[s].chords[c] then
          scale_string = scale_string .. ", "
        end
      end
      scale_string = scale_string .. "}"
      if c < #MusicUtil.SCALES[s].chords then
        scale_string = scale_string .. ", "
      end
    end
    scale_string = scale_string .. "}}"
    if s < #MusicUtil.SCALES then
      scale_string = scale_string .. ","
    end
    print(scale_string)

  end
end
generate_chord_lookups()
--]]

local function lookup_data(lookup_table, search)
  
  if type(search) == "string" then 
    search = string.lower(search)
    for i = 1, #lookup_table do
      if string.lower(lookup_table[i].name) == search then
        search = i
        break
      elseif lookup_table[i].alt_names then
        local found = false
        for j = 1, #lookup_table[i].alt_names do
          if string.lower(lookup_table[i].alt_names[j]) == search then
            search = i
            found = true
            break
          end
        end
        if found then break end
      end
    end
  end
  
  return lookup_table[search]
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
-- @tparam integer root_num MIDI note number (0-127) where scale will begin.
-- @tparam string scale_type String defining scale type (eg, "major", "aeolian" or "neapolitan major"), see class for full list.
-- @tparam[opt] integer octaves Number of octaves to return, defaults to 1.
-- @treturn {integer...} Array of MIDI note numbers.
function MusicUtil.generate_scale(root_num, scale_type, octaves)
  if type(root_num) ~= "number" or root_num < 0 or root_num > 127 then return nil end
  scale_type = scale_type or 1
  octaves = octaves or 1
  
  local scale_data = lookup_data(MusicUtil.SCALES, scale_type)
  if not scale_data then return nil end
  local length = octaves * #scale_data.intervals - (util.round(octaves) - 1)
  
  return generate_scale_array(root_num, scale_data, length)
end

--- Generate given number of notes of a scale from a root note.
-- @tparam integer root_num MIDI note number (0-127) where scale will begin.
-- @tparam integer scale_type String defining scale type (eg, "major", "aeolian" or "neapolitan major"), see class for full list.
-- @tparam integer length Number of notes to return, defaults to 8.
-- @treturn {integer...} Array of MIDI note numbers.
function MusicUtil.generate_scale_of_length(root_num, scale_type, length)
  length = length or 8
  
  local scale_data = lookup_data(MusicUtil.SCALES, scale_type)
  if not scale_data then return nil end
  
  return generate_scale_array(root_num, scale_data, length)
end


--- Generate chord from a root note.
-- @tparam integer root_num MIDI note number (0-127) for chord.
-- @tparam string chord_type String defining chord type (eg, "major", "minor 7" or "sus4"), see class for full list.
-- @tparam[opt] integer inversion Number of chord inversion.
-- @treturn {integer...} Array of MIDI note numbers.
function MusicUtil.generate_chord(root_num, chord_type, inversion)
  if type(root_num) ~= "number" or root_num < 0 or root_num > 127 then return nil end
  chord_type = chord_type or 1
  inversion = inversion or 0
  
  local chord_data = lookup_data(MusicUtil.CHORDS, chord_type)
  if not chord_data then return nil end

  local out_array = {}
  for i = 1, #chord_data.intervals do
    local note_num = root_num + chord_data.intervals[i]
    if note_num > 127 then break end
    table.insert(out_array, note_num)
  end

  for i = 1, util.clamp(inversion, 0, #out_array - 1) do
    local head = table.remove(out_array, 1)
    table.insert(out_array, head + 12)
  end

  return out_array
end

--- List chord types for a given root note and key.
-- @tparam integer note_num MIDI note number (0-127) for root of chord.
-- @tparam integer key_root MIDI note number (0-127) for root of key.
-- @tparam string key_type String defining key type (eg, "major", "aeolian" or "neapolitan major"), see class for full list.
-- @treturn {string...} Array of chord type strings that fit the criteria.
function MusicUtil.chord_types_for_note(note_num, key_root, key_type)

  if type(key_root) ~= "number" or key_root < 0 or key_root > 127 then return nil end
  key_type = key_type or 1
  local scale_data = lookup_data(MusicUtil.SCALES, key_type)
  if not scale_data then return nil end

  local position_in_scale
  for i = 1, #scale_data.intervals do
    if scale_data.intervals[i] == (note_num - key_root) % 12 then
      position_in_scale = i
      break
    end
  end

  local out_array = {}
  if position_in_scale then
    for i = 1, #scale_data.chords[position_in_scale] do
      table.insert(out_array, MusicUtil.CHORDS[scale_data.chords[position_in_scale][i]].name)
    end
  end
  return out_array
end


--- Snap a MIDI note number to the nearest note number in an array.
-- @tparam integer note_num MIDI note number input (0-127).
-- @tparam {integer...} snap_array Array of MIDI note numbers to snap to, must be in low to high order.
-- @treturn integer Adjusted note number.
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
-- @tparam {integer...} note_nums_array Array of input MIDI note numbers.
-- @tparam {integer...} snap_array Array of MIDI note numbers to snap to, must be in low to high order.
-- @treturn {integer...} Array of adjusted note numbers.
function MusicUtil.snap_notes_to_array(note_nums_array, snap_array)
  for i = 1, #note_nums_array do
    note_nums_array[i] = MusicUtil.snap_note_to_array(note_nums_array[i], snap_array)
  end
  return note_nums_array
end


--- Return a MIDI note number's note name.
-- @tparam integer note_num MIDI note number (0-127).
-- @tparam[opt] boolean include_octave Include octave number in return string if set to true.
-- @treturn string Name string (eg, "C♯3").
function MusicUtil.note_num_to_name(note_num, include_octave)
  local name = MusicUtil.NOTE_NAMES[note_num % 12 + 1]
  if include_octave then name = name .. math.floor(note_num / 12 - 2) end
  return name
end

--- Return an array of MIDI note numbers' names.
-- @tparam {integer...} note_nums_array Array of MIDI note numbers.
-- @tparam[opt] boolean include_octave Include octave number in return strings if set to true.
-- @treturn {string...} Array of name strings.
function MusicUtil.note_nums_to_names(note_nums_array, include_octave)
  local out_array = {}
  for i = 1, #note_nums_array do
    out_array[i] = MusicUtil.note_num_to_name(note_nums_array[i], include_octave)
  end
  return out_array
end


--- Return a MIDI note number's frequency.
-- @tparam integer note_num MIDI note number (0-127).
-- @treturn float Frequency number in Hz.
function MusicUtil.note_num_to_freq(note_num)
  return 13.75 * (2 ^ ((note_num - 9) / 12))
end

--- Return an array of MIDI note numbers' frequencies.
-- @tparam {integer...} note_nums_array Array of MIDI note numbers.
-- @treturn {float...} Array of frequency numbers in Hz.
function MusicUtil.note_nums_to_freqs(note_nums_array)
  local out_array = {}
  for i = 1, #note_nums_array do
    out_array[i] = MusicUtil.note_num_to_freq(note_nums_array[i])
  end
  return out_array
end


--- Return a frequency's nearest MIDI note number.
-- @tparam float freq Frequency number in Hz.
-- @treturn integer MIDI note number (0-127).
function MusicUtil.freq_to_note_num(freq)
  return util.clamp(math.floor(12 * math.log(freq / 440.0) / math.log(2) + 69.5), 0, 127)
end

--- Return an array of frequencies' nearest MIDI note numbers.
-- @tparam {float...} freqs_array Array of frequency numbers in Hz.
-- @treturn {integer...} Array of MIDI note numbers.
function MusicUtil.freqs_to_note_nums(freqs_array)
  local out_array = {}
  for i = 1, #freqs_array do
    out_array[i] = MusicUtil.freq_to_note_num(freqs_array[i])
  end
  return out_array
end


--- Return the ratio of an interval.
-- @tparam float interval Interval in semitones.
-- @treturn float Ratio number.
function MusicUtil.interval_to_ratio(interval)
  return math.pow(2, interval / 12)
end

--- Return an array of ratios of intervals.
-- @tparam {float...} intervals_array Array of intervals in semitones.
-- @treturn {float...} Array of ratio numbers.
function MusicUtil.intervals_to_ratios(intervals_array)
  local out_array = {}
  for i = 1, #intervals_array do
    out_array[i] = MusicUtil.interval_to_ratio(intervals_array[i])
  end
  return out_array
end

--- Return the interval of a ratio.
-- @tparam float ratio Ratio number.
-- @treturn float Interval in semitones.
function MusicUtil.ratio_to_interval(ratio)
  return 12 * math.log(ratio) / math.log(2)
end

--- Return an array of intervals of ratios.
-- @tparam {float...} ratios_array Array of ratio numbers.
-- @treturn {float...} Array of intervals in semitones.
function MusicUtil.ratios_to_intervals(ratios_array)
  local out_array = {}
  for i = 1, #ratios_array do
    out_array[i] = MusicUtil.ratio_to_interval(ratios_array[i])
  end
  return out_array
end


return MusicUtil
