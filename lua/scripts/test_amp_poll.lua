require 'math'

print("test_amp_poll.lua")

local function printAsciiMeter(amp, n, floor)
   n = n or 64
   floor = floor or -72
   local db = 20.0 * math.log10(amp)
   local norm = 1.0 - (db / floor)
   local x = norm * n
   local str = ""
   for i=0,x do
      str = str.."#"
   end
   print(str)
end

poll = function(poll, value)
   if poll.name == "amp_in_l" then
      printAsciiMeter(value)
   end
end

report.polls = function()
   if norns.polls['amp_in_l'] then
	 -- FIXME: not a great interface.
	 -- poll objects should have "set_time". "start", "stop" methods...
	 norns.set_poll_time('amp_in_l', 0.05)
	 norns.start_poll('amp_in_l');
   end
end

report_polls()
