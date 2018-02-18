// an audio "engine."
// maintains some DSP processing and provides control over parameters and analysis results
// new engines should inherit from this
CroneEngine {
	// an AudioContext
	var <context;

	// list of registered commands
	var <commands;
	var <commandNames;

	// list of registered parameters
	var <parameters;
	var <parameterNames;
	var <parameterControlBusses;

	// list of registered polls
	var <pollNames;

	*new { arg context;
		^super.new.init(context);
	}

	init { arg argContext;
		commands = List.new;
		commandNames = IdentityDictionary.new;
		parameters = List.new;
		parameterNames = IdentityDictionary.new;
		parameterControlBusses = IdentityDictionary.new;
		pollNames = Set.new;
		context = argContext;
	}

	addPoll { arg name, func;
		name = name.asSymbol;
		CronePollRegistry.register(name, func);
		pollNames.add(name);
	}

	// NB: subclasses should override this if they need to free resources
	// but the superclass method should be called as well
	free {
		postln("CroneEngine.free");
		commands.do({ arg com;
			com.oscdef.free;
		});
		parameters.do({ arg parameter;
			parameter.oscdef.free;
		});
		parameterControlBusses.do({ arg cbus;
			cbus.free;
		});
		pollNames.do({ arg name;
			CronePollRegistry.remove(name);
		});
	}

	addCommand { arg name, format, func;
		var idx, cmd;
		name = name.asSymbol;
		// TODO: check its a unique commands+parameters name
		postln([ "CroneEngine adding command", name, format, func ]);
		if(commandNames[name].isNil, {
			idx = commandNames.size;
			commandNames[name] = idx;
			cmd = Event.new;
			cmd.name = name;
			cmd.format = format;
			cmd.oscdef = OSCdef(name.asSymbol, {
				arg msg, time, addr, rxport;
				// ["CroneEngine rx command", msg, time, addr, rxport].postln;
				func.value(msg);
			}, ("/command/"++name).asSymbol);
			commands.add(cmd);
		}, {
			idx = commandNames[name];
		});
		^idx
	}

	addParameter { arg name, spec;
		var idx;
		name = name.asSymbol;
		// TODO: check its a unique commands+parameters name
		postln([ "CroneEngine adding parameter", name, spec ]);
		if(parameterNames[name].isNil, {
			var bus;
			idx = parameterNames.size;
			parameterNames[name] = idx;
			bus = Bus.control;
			bus.setSynchronous(spec.default);
			parameterControlBusses[name] = bus;
			parameters.add(
				(
					name: name,
					spec: spec,
					oscdef: OSCdef(name.asSymbol, { // TODO: why not OSCFunc here?
						arg msg, time, addr, rxport;
						// ["CroneEngine rx parameter", msg, time, addr, rxport].postln;
						bus.setSynchronous(spec.constrain(msg[1].asFloat));
					}, ("/parameter/"++name).asSymbol)
				)
			);
		}, {
			idx = parameterNames[name];
		});
		^idx
	}
}

