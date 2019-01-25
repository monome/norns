// Engine_Sines
// a lot of sines
Engine_Sines : CroneEngine {
	classvar num;
	var <synth;

	*initClass {  num = 64; }

	*new { arg context, doneCallback;
		^super.new(context, doneCallback);
	}

	alloc {
		var server = Crone.server;
		var def = SynthDef.new(\sine, {
			arg out, hz=220, amp=0.0, amp_atk=0.001, amp_rel=0.05, hz_lag=0.005, pan=0, pan_lag=0.005;
			var amp_, hz_, pan_;
			amp_ = LagUD.ar(K2A.ar(amp), amp_atk, amp_rel);
			hz_ = Lag.ar(K2A.ar(hz), hz_lag);
			pan_ = Lag.ar(K2A.ar(pan), pan_lag);
			Out.ar(out, Pan2.ar(SinOsc.ar(hz_) * amp_, pan));
		});
		def.send(server); 
		server.sync;
		
		synth = Array.fill(num, { Synth.new(\sine, [\out, context.out_b], target: context.xg) });

		#[\hz, \amp, \pan, \amp_atk, \amp_rel, \hz_lag, \pan_lag].do({
			arg name;
			this.addCommand(name, "if", {
				arg msg;
				var i = msg[1] -1;
				if(i<num && i >= 0, { 
					synth[msg[1]].set(name, msg[2]);
				});
			});
		});

	}

	free {
		synth.do({ |syn| syn.free; });
	}
}