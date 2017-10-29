print('test_engine.lua')

norns = require 'norns'
local engine = require 'engine'


engine.load('TestSine',
	    function(commands, count)
	       print("commands: ")
	       for i,v in pairs(commands) do
		  print(i, v.fmt)
	       end
	       engine.hz(100)
	       engine.amp(0.125)
	    end
)
