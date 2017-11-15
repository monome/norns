print('test_engine.lua')

norns = require 'norns'
e = require 'engine'


e.load('TestSine',
	    function(commands, count)
	       print("commands: ")
	       for i,v in pairs(commands) do
		  print(i, v.fmt)
	       end
	       e.hz(100)
	       e.amp(0.125)
	    end
)
