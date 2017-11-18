print("test_amp_poll.lua")
require 'math'
require 'norns'
local poll = require 'poll'

local p = nil

norns.script.cleanup = function()
   if p then p:stop() end
end

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

local ampCallback = function(amp) printAsciiMeter(amp, 64, -36) end

poll.report = function(polls)
   print("available polls: ")
   for _,p in pairs(polls) do
      print("",p.name)
   end   
   p = polls['amp_in_r']
   if p then
      p.callback = ampCallback
      p.time = 0.03;
      p:start()
   else
      print("couldn't get requested poll, dang")
   end 
end

report_polls()
