// Pluck uGen workout. Shout me @burn on llllllll.co if you need any assistance.

Engine_KarplusRings : CroneEngine {
	var pg;
	var amp=0.1;
	var freq = 440;
	var decay = 5;
	var coef = 0.1;
  var lpf_freq = 3000;
	var lpf_gain = 1;
	var bpf_freq = 2000;
	var bpf_res = 0.3;

		*new { arg context, doneCallback;
		^super.new(context, doneCallback);
	}

	alloc {
		pg = ParGroup.tail(context.xg);

		SynthDef("karplus_rings", {arg out, amp = amp, freq = freq, decay = decay, coef = coef, lpf_freq = lpf_freq, lpf_gain = lpf_gain, bpf_freq = bpf_freq, bpf_res = bpf_res;
			var env, snd;
			env = EnvGen.kr(Env.linen(0, decay, 0), doneAction: 2);
			snd = Pluck.ar(
				in: BPF.ar(in:WhiteNoise.ar(amp), freq: bpf_freq, rq: bpf_res),
				trig: Impulse.kr(0),

				maxdelaytime: 0.1,
				delaytime: freq.reciprocal,
				decaytime: decay,
				coef: coef);
			Out.ar(out, Pan2.ar(MoogFF.ar(in: snd, freq: lpf_freq, gain: lpf_gain)));
		}).play(args: [\out, context.out_b], target: pg);

		this.addCommand("hz", "f", { arg msg;
			var val = msg[1];
            Synth("karplus_rings", [\out, context.out_b, \freq,val,\amp,amp,\decay,decay,\coef,coef,\lpf_freq,lpf_freq,\lpf_gain,lpf_gain,\bpf_freq,bpf_freq,\bpf_res,bpf_res], target:pg);
		});

		this.addCommand("amp", "f", { arg msg;
			amp = msg[1];
		});

		this.addCommand("decay", "f", { arg msg;
			decay = msg[1];
		});

		this.addCommand("coef", "f", { arg msg;
			coef = msg[1];
		});

		this.addCommand("lpf_freq", "f", { arg msg;
			lpf_freq = msg[1];
		});

		this.addCommand("lpf_gain", "f", { arg msg;
			lpf_gain = msg[1];
		});

		this.addCommand("bpf_freq", "f", { arg msg;
			bpf_freq = msg[1];
		});

		this.addCommand("bpf_res", "f", { arg msg;
			bpf_res = msg[1];
		});
	}

}
