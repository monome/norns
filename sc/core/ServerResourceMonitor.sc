ServerResourceMonitor {

	var <server;

	var <current;
	var <baseline;

	*new { arg aServer;
		^super.new.init(aServer);
	}

	init {
		arg aServer;
		server = aServer;
		current = Dictionary.new;
		baseline = Dictionary.new;
	}

	delta {
		var l = Dictionary.new;
		var d;
		current.keysValuesDo({ arg k, v;
			[[k, v], [k, this.baseline[k]]].postln;
			d = v - baseline[k];
			if (d != 0, { l[k] = d });
		});
		^l
	}

	refresh {
		current[\synth] = server.numSynths;
		current[\group] = server.numGroups;
		current[\bus_control] = server.controlBusAllocator.totalUsed;
		current[\bus_audio] = server.audioBusAllocator.totalUsed;
		current[\buffer] = server.bufferAllocator.totalUsed;
		^this.delta
	}

	setBaseline {
		baseline[\synth] = server.numSynths;
		baseline[\group] = server.numGroups;
		baseline[\bus_control] = server.controlBusAllocator.totalUsed;
		baseline[\bus_audio] = server.audioBusAllocator.totalUsed;
		baseline[\buffer] = server.bufferAllocator.totalUsed;
	}


}