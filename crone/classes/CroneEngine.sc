// an audio "engine." only one exists at a time.
CroneEngine {
	var server;
	var bufs;
	var group;
	
	var >params;
	var >paramNames;

	*new { arg serv;
		^super.new.init(serv);
	}

	init { arg serv;
		server = serv;
		bufs = List.new;
		group = Group.new(server);
		params = List.new;
		paramNames = Dictionary.new;
	}
	
	kill {
		bufs.do({ arg ev; ev.buf.free; });
		group.free;
	}
	
	addParam { arg name, val, func;
		var idx = 0; 
		if(paramNames[name].isNil, {
			idx = paramNames.size;
			paramNames[name] = idx;
			params.add( (\name:name, \val:val, \func:func) );
		});
		^idx
	}

	addBuffer { arg name, duration, channels;
		bufs.add( (\name: name, \buf:
			Buffer.alloc(server, duration * server.sampleRate, channels);
		));
	}
	
	setParamByIndex { arg idx, val;
		^params[idx].func.value(params[idx].val);
	}

	setParamByName { arg name, val;
		var param = params[paramNames[name]];
		^param.func.value(param.val);
	}
	
	getParamByIndex { arg idx;
		^(params[idx].val);
	}

	getParamByName { arg name;
		^(params[paramNames[name]].val);
	}

}
