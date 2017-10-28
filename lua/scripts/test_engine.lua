local norns = require 'norns'
local engine = require 'engine'


engine.load('TestSine',
	    function(commands, count)
	       print("commands: ")
	       for i,v in pairs(commands) do
		  print(i, v.fmt)
	       end

	    end
)
