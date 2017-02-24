// class to perform crossfaded playback cuts
CutFadeVoice {

	var <buf;  // single audio buffer (reference)
	var <trig_b; // array of trigger busses. setting a trigger causes a jump in position
	var <play_b; // output bus
	
	var <play_s;  // pair of playback synths
	var <out_s; // output synth
	
	var <pos;  // position of last cut
	var <rate; // current playback rate

	var <fadeTime; // fade time
	var <pan; // output pan poition
	var <mute; // output mute
	var <level; // output level
	
	var sw; // switch read heads
	var t; // time marker

	*new { arg srv, buf_, play_target, patch_target, out_bus;
		^super.new.init(srv, buf_, play_target, patch_target, out_bus);
	}
	
	init { arg srv, buf_, play_target, patch_target, out_bus;
		play_b = Bus.audio(srv, 1);
		buf = buf_;
		
		trig_b = Array.fill(2, { Bus.control(srv) });
		play_s = Array.fill(2, { arg i;
			Synth.new(\play_fade, [
				\buf, buf.bufnum,
				\gate, 0,
				\trig, trig_b[i].index,
				\out, play_b.index
			], play_target)
		});

		
		out_s = Synth.new(\patch_pan, [
			\in, play_b.index,
			// FIXME: should use output bus arg.
			// \out, out_bus.index,
			/// for now, patch output directly to DAC
			\out, 0 
		], patch_target);
		
		
		level = 1.0;
		mute = 1.0;
		pan = 0;
		rate = 1;
		pos = 0;

		sw = 0;
		t = 0;
	}

	// change position with crossfade
	pos_ { arg p;
		pos = p;
		t = SystemClock.seconds;
		play_s[sw].set(\start, pos * buf.sampleRate, \rate, rate);
		trig_b[sw].set(1);
		play_s[sw].set(\gate, 1);
		play_s[1-sw].set(\gate, 0);
		sw = 1-sw;
	}

	// change playback rate with crossfade
	rate_ { arg r;
		play_s[sw].set(\start, (
			pos + (SystemClock.seconds - t)) * buf.sampleRate * rate);
		rate = r;
		trig_b[sw].set(1);
		play_s[sw].set(\rate, rate);
		play_s[sw].set(\gate, 1);
		play_s[1-sw].set(\gate, 0);
		sw = 1-sw;
	}
	
		// set fade time
	fadeTime_ { arg id, x;
		play_s.do({ arg syn; syn.set(\amp_fade_time, x); });
	}

	// pan position
	pan_ { arg x;
		out_s.set(\pan, x);
	}
	
	// play mute
	mute_ { arg x;
		out_s.set(\mute, x);
	}
	
	// play level
	level_ { arg x;
		out_s.set(\level, x);
	}

	// toggle looping behavior on the entire buffer
	loop_ { arg x;
		play_s.do({ |syn| syn.set(\loop, x) });
	}
	
	// set the loop interval
	length_ { arg x;
		// TODO
	}
	
}


