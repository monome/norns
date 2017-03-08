--[[
   stickcut.lua
   trivial demo showing how to use the 'Cutter' engine
--]]

version.stickcut = "0.0.1"

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

   -- load a soundfile into first buffer
   e.read(1, '/snd/hurt.wav') -- oh yes i did
   -- twll the first 4 playback voices to use the first buffer
   e.buf(1, 1)
   e.buf(2, 1)
   e.buf(3, 1)
   e.buf(4, 1)

   -- set the first 4 voices to different playback rates
   e.rate(1, 0.5)
   e.rate(2, 1.0)
   e.rate(3, 1.25)
   e.rate(4, 1.5)

   -- global: position to jump to in the buffer
   pos = 0.0;

   joystick.axis = function(stick, ax, val)
	  if(ax == 1) then
		 pos = ((val / 32767) + 1.0) * 160.
		 print("pos: " .. pos)
	  end
   end

   joystick.button = function(stick, but, val)
	  print("but: " .. but .. " " .. val)
	  if(but < 5) then
		 if(val > 0) then
			e.pos(but, pos)
			e.mute(but, 0)
		 else
			e.mute(but, 1)
		 end
	  end
   end

end -- report handler

-- handler is set; load the engine
load_engine('Cutter')
