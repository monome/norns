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
		
		synth = Array.fill(num, {
			{
				arg out, hz=220, amp=0.0, amplag=0.01, hzlag=0.01, pan=0, panlag=0.01;
				var amp_, hz_, pan_;
				amp_ = Lag.ar(K2A.ar(amp), amplag);
				hz_ = Lag.ar(K2A.ar(hz), hzlag);
				pan_ = Lag.ar(K2A.ar(pan), panlag);
				Out.ar(out, Pan2.ar(SinOsc.ar(hz_) * amp_, pan));
			}.play(args: [\out, context.out_b], target: context.xg);
		});
			

		this.addCommand("hz", "if", { arg msg;
			synth[msg[1]].set(\hz, msg[2]);
		});

		this.addCommand("amp", "if", { arg msg;
			synth[msg[1]].set(\amp, msg[2]);
		});
	}

	free {
		synth.do({ |syn| syn.free; });
		super.free;
	}
}
