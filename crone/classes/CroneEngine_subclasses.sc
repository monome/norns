// Crone_TestSine
// dumbest possible test: a single, mono sinewave
Crone_TestSine : CroneEngine {
	var <synth;

	*new { arg serv_;
		^super.new.initSub(serv_);
	}

	initSub {
		synth = {
			arg hz=220, amp=0.5, amplag=0.1, hzlag=0.1;
			var amp_, hz_;
			amp_ = Lag.kr(amp, amplag);
			hz_ = Lag.kr(hz, hz);
			Out.ar(0, (SinOsc.ar(hz) * amp).dup);
		}.play(this.group);

		this.addParam("hz", 110.0, { arg param;
			postln("set hz : " ++ param);
			synth.set(\hz, param.val);
		});

		this.addParam("amp", 0.0, { arg param;
			postln("set amp : " ++ param);
			synth.set(\amp, param.val);
		});
	}
}
