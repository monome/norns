// class to perform crossfaded playback cuts
CutFadeVoice {

	var <buf;  // single audio buffer number
	var <trig_b; // array of trigger busses. setting a trigger causes a jump in position
	var <out_b; // output bus
	
	var <play_s;  // pair of playback synths
	var <pos;  // position of last cut
	var <rate; // current playback rate

	var <fadeTime; // fade time
	var <pan; // output pan position
	var <mute; // output mute
	var <level; // output level

	
	var sw; // switch read heads
	var t; // time marker
	
	*new { arg srv, buf_, target;
		^super.new.init(srv, buf_);
	}

	init { arg srv, buf_, target;
		out_b = Bus.audio(srv, 1);
		buf = buf_;
		
		trig_b = Array.fill(2, { Bus.control(srv) });
		play_s = Array.fill(2, { arg i;
			Synth.new(\play_fade, [
				\buf, buf.bufnum,
				\gate, 0,
				\trig, trig_b[i].index,
				\out, out_b.index,
				\curve: \sine
			], target)
		});
		
		level = 1.0;
		mute = 0;
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
	fade_ { arg id, x;
		play_s.do({ arg syn; syn.set(\fade_time, x); });
	}

	// set fade curve
	curve_ { arg id, x;
		play_s.do({ arg syn; syn.set(\fade_curve, x); });
	}
	
	// play mute
	mute_ { arg x;
		mute = x;
		play_s.do({ arg syn; syn.set(\mute, mute) });
	}
	
	// play level
	level_ { arg x;
		level = x;
		play_s.do({ arg syn; syn.set(\level, level) });
	}

	// toggle raw looping behavior on the entire buffer
	loop_ { arg x;
		play_s.do({ |syn| syn.set(\loop, x) });
	}
	
	// set the (crossfaded) loop interval
	length_ { arg x;
		// TODO... no autonomous, xfaded loop mechanism yet
	}

	// set the buffer
	buffer_ { arg buf_;
		buf = buf_;
		play_s.do({ |syn| syn.set(\buf, buf) });
	}
	
}


