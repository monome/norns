// test of client-side fade switching

Routine {
	s = Server.default;
	s.waitForBoot({
		SynthDef.new(\proto_play_fade, {
			arg out=0, buf=0, start=0, end=9999, trig=0, rate=1.0,
			amp, amp_lag=0.2, amp_lag_shape=0.0;
			var phase, snd, aenv;
			phase = Phasor.ar(InTrig.kr(trig), BufRateScale.kr(buf) * rate, 0, end, start);
			snd =  BufRd.ar(1, buf, phase);
			//aenv = VarLag.ar(K2A.ar(amp), amp_lag, amp_lag_shape);
			// FIXME: varlag seems broken, use Env i guess
			aenv = Lag.ar(K2A.ar(amp), amp_lag);

			Out.ar(out, (snd * aenv).dup);
		}).send(s);
		s.sync;

//		~buf_dur = 16.0;
//		~buf_frames = ~buf_dur * s.sampleRate;
//		~buf = Buffer.alloc(s, ~buf_frames, 1);
// 		~buf.read("hurt.wav", ~buf_frames);

		~buf = Buffer.read(s, Document.current.path.dirname ++ "/hurt.wav");
		s.sync;
		~buf_frames = ~buf.numFrames;
		("buf_frames: " ++ ~buf_frames).postln;

		~play_g = Group.new(s);
		s.sync;

		// fixme: using event as "pseudo-class" for now
		// better to make a real class
		~voice_factory = {
			var buf, fade_voice, trig_bus;
			trig_bus = Array.fill(2, { Bus.control(s) });
			fade_voice = (
				buf: buf,
				trig:trig_bus,
				syn: Array.fill(2, { |i|
					Synth.new(\proto_play_fade, [
						\buf, ~buf.bufnum,
						\end, ~buf_frames,
						\amp, 0.0,
						\trig, trig_bus[i].index
					], ~play_g)
				}),
				target: 0,
				pos: 0,
				t: 0.0
			);
			fade_voice
		};

		// just make a single voice for now
		~voice = ~voice_factory.value;
		s.sync;

		// set position and switch fades.
		~voice_set_pos = { arg voice, pos;
			//(voice.synth).postln;
			postln("setting pos : " ++ pos );
			postln("target: " ++ voice.target);
			voice.pos = pos;
			voice.t = SystemClock.seconds;
			voice.syn[voice.target].set(\start, pos * ~buf.sampleRate);
			voice.trig[voice.target].set(1);
			voice.syn[voice.target].set(\amp, 1.0);
			voice.syn[1 - voice.target].set(\amp, 0.0);
			voice.target = 1 - voice.target;
			postln("next target: " ++ voice.target);
			postln("");
		};


		// set playback rate and switch fades.
		~voice_set_rate = { arg voice, rate;
			postln("setting rate : " ++ rate );
			// i dunno
			voice.syn[voice.target].set(\start, (
				voice.pos + (SystemClock.seconds - voice.t)) * ~buf.sampleRate);
			voice.trig[voice.target].set(1);
			voice.syn[voice.target].set(\rate, rate);
			voice.syn[voice.target].set(\amp, 1.0);
			voice.syn[voice.target].set(\amp, 0.0);
		};

		{ ~gui.value(~voice) }.defer;
	}); // waitForBoot
}.play;


// quick test gui
~gui = { arg voice;

	~getkey = { arg qt;
		var val = qt.switch(
			// 4 rows of 3: "123", "qwe", "asd", "zxc" in US
			49, {0},
			81, {1},
			65, {2},
			50, {3},
			87, {4},
			83, {5},
			51, {6},
			69, {7},
			68, {8},
			90, {9},
			88, {10},
			67, {11},
			16777220, {12}, // enter
			32, {666}, // space
		);
		val
	};

	w = Window.new("", Rect(0, 0, 512, 512));
	w.front;
	w.view.keyDownAction = { arg view, char, mod, uni, key, qt;
		var val;
		[view, char, mod, uni, key, qt].postln;
		val = ~getkey.value(qt);

		if( val.notNil, {
			if( val>=0 && val<12 , {
				~voice_set_pos.value(voice, val);
			});
		});

	};


};
