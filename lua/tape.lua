local Tape = {}

Tape.tape_play_open = function(file)
  _norns.tape_play_open(file)
end

Tape.tape_play_start = function()
  _norns.tape_play_start()
end

Tape.tape_play_stop = function()
  _norns.tape_play_stop()
end

Tape.tape_record_open = function(file)
  _norns.tape_record_open(file)
end

Tape.tape_record_start = function()
  _norns.tape_record_start()
end

Tape.tape_record_stop = function()
  _norns.tape_record_stop()
end

return Tape
