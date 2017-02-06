// an audio "engine." only one exists at a time.
CroneEngine {
	var <server;
	var <group;

	var <params;
	var <paramNames;

	var <bufs;
	var <bufNames;

	*new { arg serv;
		^super.new.init(serv);
	}

	init { arg serv;
		server = serv;
		group = Group.new(server);
		params = List.new;
		paramNames = Dictionary.new;
		bufs = List.new;
		bufNames = Dictionary.new;
	}

	kill {
		// TODO: let the subclasses decide how to do this more gracefully
		bufs.do({ arg ev; ev.buf.free; });
		group.free;
	}

	// add a parameter.
	// name: name (string)
	// val: initial value (a number)
	// func: handler function. takes a single argument (the parameter structure)
	addParam { arg name, val, func;
		var idx = 0;
		name = name.asSymbol;
		if(paramNames[name].isNil, {
			idx = paramNames.size;
			paramNames[name] = idx;
			params.add( (\name:name, \val:val, \func:func) );
		});
		^idx
	}

	addBuffer { arg name, duration, channels;
		var idx = 0;
		name = name.asSymbol;
		if(bufNames[name].isNil, {
			idx = bufNames.size;
			bufNames[name] = idx;
			bufs.add( (\name: name, \buf:
				Buffer.alloc(server, duration * server.sampleRate, channels);
			));
		});
	}

	loadBufferByName { arg name, path;
		bufs[bufNames[name]].read(path);
	}

	setParamByIndex { arg idx, val;
		params[idx].val = val;
		^params[idx].func.value
	}

	setParamByName { arg name, val;
		var param, idx;
		name = name.asSymbol;
		idx = paramNames[name];
		this.setParamByIndex(idx, val);
	}

	getParamByIndex { arg idx;
		^(params[idx].val);
	}

	getParamByName { arg name;
		^(params[paramNames[name]].val);
	}

}

