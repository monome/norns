// CroneEngine_PolyPerc
// sine with perc envelopes, triggered on freq
CroneEngine_PolyPerc : CroneEngine {
    var amp=0.3;
    var release=0.5;
    var pw=0.5;
    var cutoff=1000;
    var gain=2;

	*new { arg server, group, in, out;
		^super.new.init(server, group, in, out).initSub();
	}

	initSub {
        SynthDef("PolyPerc", {arg freq = 440, pw=pw, amp=amp, cutoff=cutoff, gain=gain, release=release;
			var snd = LFPulse.ar(freq, 0, pw);
			var filt = MoogFF.ar(snd,cutoff,gain);
			var env = Env.perc(level: amp, releaseTime: release).kr(2);
			Out.ar(0, (filt*env).dup);
		}).add; 
		
		this.addCommand("hz", "f", { arg msg;
			var val = msg[1];
            Synth("PolyPerc", [\freq,val,\pw,pw,\amp,amp,\cutoff,cutoff,\gain,gain,\release,release]);
		});

		this.addCommand("amp", "f", { arg msg;
			amp = msg[1];
		});

		this.addCommand("pw", "f", { arg msg;
			pw = msg[1];
		});
		this.addCommand("release", "f", { arg msg;
			release = msg[1];
		});
		this.addCommand("cutoff", "f", { arg msg;
			cutoff = msg[1];
		});
		this.addCommand("gain", "f", { arg msg;
			gain = msg[1];
		});
	}
}
