// CroneEngine_TestSine
// dumbest possible test: a single, mono sinewave
CroneEngine_TestSine : CroneEngine {
	var <synth;

	*new { arg serv_;
		^super.new.initSub(serv_);
	}

	initSub {
		synth = {
			arg hz=220, amp=0.5, amplag=0.02, hzlag=0.01;
			var amp_, hz_;
			amp_ = Lag.ar(K2A.ar(amp), amplag);
			hz_ = Lag.ar(K2A.ar(hz), hzlag);
			Out.ar(0, (SinOsc.ar(hz_) * amp_).dup);

		}.play(this.group);
		
		this.addCommand("hz", "f", { arg msg;
			var val = msg[1];
			postln("set hz : " ++ val);
			synth.set(\hz, val);
			
		});

		this.addCommand("amp", "f", { arg msg;
			var val = msg[1];
			postln("set amp : " ++ val);
			synth.set(\amp, val);
		});
	}
}