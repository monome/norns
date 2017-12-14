e = require 'engine' 

--e.load('Cutter',
--e.load('TestSine',
e.load('PolyPerc',
	    function(commands, count)
	       print("commands: ")
	       for i,v in pairs(commands) do
		  print(i, v.fmt)
	       end
	       e.hz(100)
	       e.amp(0.125)
	    end
)
