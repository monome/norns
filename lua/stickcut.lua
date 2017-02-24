--[[

   stickcut.lua

--]]

print("\n --- \n hello stickcut.lua \n --- \n")

--[[
   we need to load the engine, then wait for the engine to be ready.
   should probably add an explicit mechanism for this;
   for now, it will work to just use the command report handler
--]]
report.commands = function(commands, count)
   print("handling command report... \n")
   addEngineCommands(commands, count)
   e = engine;
   
   -- load a soundfile into buffer 0
   e.read(0, '/snd/hurt.wav') -- oh yes i did
   e.read(1, '/snd/hurt.wav')
   e.read(2, '/snd/hurt.wav')
   e.read(3, '/snd/hurt.wav')

   -- which position to jump to in the buffer
   pos = 0.0;

   -- fixme (?) the engine takes 0-based indices, which is confusing
   -- set the first 4 voices to different playback rates, hoo yeah
   e.rate(0, 0.5)
   e.rate(1, 1.0)
   e.rate(2, 1.25)
   e.rate(3, 1.5)

   joystick.axis = function(stick, ax, val)
	  if(ax == 1) then
		 pos = ((val / 32767) + 1.0) * 16.0
		 print("pos: " .. pos)
	  end
   end

   joystick.button = function(stick, but, val)
	  print("but: " .. but .. " " .. val)
	  if(val > 0) then
		 -- fixme: should indices be converted to 0-base later? hm.. 
		 e.pos(but-1, pos)
		 e.mute(but-1, 1.0)
	  else
		 e.mute(but-1, 0.0)
	  end
   end
   
end -- report handler

-- load the engine
load_engine('Cutter')
