//--- leave undefined,, start with a blank 1m buffer
~soundfilename = nil;

//... or load a file
//~soundfilename = "vla_test_48.wav";


s.waitForBoot { Routine {


	~buf = if(~sndFilename.notNil, {
		Buffer.read(s, PathName(Document.current.path).pathOnly ++ ~soundfilename);
	}, {
		Buffer.alloc(s, s.sampleRate * 10.0, 1);
	});

	s.sync;

	postln("buffer samplerate: " ++ ~buf.sampleRate ++  " ; duration: " ++ ~buf.duration);

	if(~buf.sampleRate != s.sampleRate, {
		postln("WARNING: buffer and server have different sample rates. recording over this buffer may produce unwanted artifacts. ");
	});

	{ s.scope }.defer;

	/*
	// looped, crossfaded playback
	SynthDef.new(\softcuthead, {
	arg buf, in, out, phase_out= -1, trig_out= -1, trig_in = -1,
	amp=0.2, rec=0.0, pre=0.0,
	rate=1, ratelag = 0.1,
	start=0, end=1, fade=0.1, loop=1,
	fadeRec=1.0, fadePre = 1.0, recRun = 0;

	var snd, phase, tr;
	var brs;
	var cutfade;
	var trig;
	var sin;

	brs = BufRateScale.kr(buf);

	trig = InTrig.kr(trig_in);
	sin = In.ar(in);

	pre = Lag.ar(K2A.ar(pre), 0.1);
	rec = Lag.ar(K2A.ar(rec), 0.1);

	rate = Lag.ar(K2A.ar(rate), ratelag);

	cutfade =  SoftCutHead.ar(buf, sin, trig,
	rate * brs, start, end, fade, loop,
	rec, pre, fadeRec, fadePre, recRun);

	phase = cutfade[0];
	tr = cutfade[1];
	snd = cutfade[2];

	Out.ar(out, (snd*amp).dup);
	Out.ar(phase_out, phase);
	// NB: this is an _audio_ rate trigger;
	// it stays high for only one sample
	// .kr ugens that read once per audio block will miss it
	Out.ar(trig_out, tr);
	}).send(s);
	*/
	SynthDef.new(\softcuthead, {
		arg buf, in, out, gate=1,
		phase_out= -1, trig_out= -1, trig_in = -1,
		amp=0.2, rec=0.0, pre=0.0,
		rate=1, ratelag=0.1,
		start=0, end=1, fade=0.1, loop=1,
		fadeRec=1.0, fadePre = 1.0, recRun=0, offset=0,
		preLag=0.0005, recLag=0.0005, envTimeScale = 1.0, done=0;

		var snd, phase, tr;
		var brs;
		var cutfade;
		var trig;
		var sin;
		var aenv;

		/// TODO: add an input for arbitrary record head.
		/// this should allow for a crossfade when heads cross
		/// using abs(distance between heads) as xfade envelope

		brs = BufRateScale.kr(buf);

		trig = InTrig.kr(trig_in);
		sin = In.ar(in);

		pre = Lag.ar(K2A.ar(pre), preLag);
		rec = Lag.ar(K2A.ar(rec), recLag);

		rate = Lag.ar(K2A.ar(rate), ratelag);

		cutfade =  SoftCutHead.ar(buf, sin, trig,
			rate * brs, start, end, fade, loop,
			rec, pre, fadeRec, fadePre, recRun, offset);

		phase = cutfade[0];
		tr = cutfade[1];
		snd = cutfade[2];

		aenv = EnvGen.ar(Env.asr(0.0001, 1, 0.005), gate,
			timeScale:envTimeScale, doneAction:done);

		Out.ar(out, ( snd * amp * aenv));
		Out.ar(phase_out, phase);
		// NB: this is an _audio_ rate trigger;
		// it stays high for only one sample
		// .kr ugens that read once per audio block will miss it
		Out.ar(trig_out, tr);
	}).send(s);


	/*

	// triggered overdub-recording from arbitrary position
	SynthDef(\rec_dub_trig_gate, {
		arg buf, in, gate=0,
		rate=1, start=0.0, end=1.0, loop=0,
		rec=1, pre=0, fade=0.01;

		var sr, brs,
		sin, sin_phase,
		phase, wr, trig,
		env_pre, env_rec;

		sr = SampleRate.ir;
		brs = BufRateScale.kr(buf); // NB: BfWr and BufWrPre are non-interpolating...
		env_rec = EnvGen.ar(Env.asr(fade, 1, fade), gate) * rec;
		env_pre = (pre * env_rec).max(1-env_rec);

		sin = In.ar(in);
		phase = Phasor.ar(gate, rate * brs, start*sr, end*sr, start);
		wr = BufWrPre.ar(sin * env_rec, buf, phase, env_pre);
	}).send(s);
*/

	s.sync;

	//--- state

	~start_pos = 0.0;
	~end_pos = 1.0;
	~fade_dur = 0.1;
	~loop_dur = ~end_pos - ~start_pos;

	~phase_rd_b= Bus.audio(s, 1);
	~trig_rd_b= Bus.audio(s, 1);

	// touching this control bus will trigger playback
	~loop_trig_b = Bus.control(s, 1);

	~loop_s = Synth.new(\softcuthead, [
		\buf, ~buf.bufnum,
		\in, Crone.ctx.in_b[1].index,
		\out, Crone.ctx.out_b.index,
		\start, ~start_pos, \end, ~end_pos,
		\phase_out, ~phase_rd_b.index,
		\trig_out, ~trig_rd_b.index,
		\trig_in, ~loop_trig_b.index,
		\fade, ~fade_dur,
		\pre, 1.0, \rec, 0.0
	], Crone.ctx.xg);

	/*
	~rec_trig_s = Synth.new(\rec_dub_trig_gate, [
		\in, Crone.ctx.in_b[0].index,
		\start, 0,
		\end, ~buf.duration,
		\buf, ~buf.bufnum,
	], s, addAction: \addToTail);
*/

	//--- methods

	~update_end_pos = { arg pos;
		~end_pos = pos.min(~buf.duration - ~fade_dur);
		[~start_pos, ~loop_dur, ~end_pos].postln;
		~loop_s.set(\end, ~end_pos);
	};

	~update_loop_dur = { arg dur;
		[dur, ~fade_dur].postln;
		~loop_dur = dur.max(~fade_dur + (10.0 / s.sampleRate));
		~update_end_pos.value(~start_pos + ~loop_dur);
	};

	~start_loop = { ~loop_trig_b.set(1); };

	~set_loop_flag = { arg val; ~loop_s.set(\loop, val); };

//	~set_rec_trig_gate = { arg val; ~rec_trig_s.set(\gate, val); };


	~set_rec_follow_gate = { arg val; ~loop_s.set(\recRun, val);
		postln("rec follow: " ++ val);
	};

	~set_loop_start = { arg pos, keepDur=true;
		~start_pos = pos;
		[~start_pos, ~loop_dur, ~end_pos].postln;
		~loop_s.set(\start, ~start_pos);
		if(keepDur, {
			~update_end_pos.value(~start_pos + ~loop_dur);
		});
	};

	~set_loop_end = { arg pos; ~update_end_pos.value(pos); };
	~set_loop_dur = { arg dur; ~update_loop_dur.value(dur); };

	//-- test gui
	{
		w = Window("", Rect(0, 0, 720, 360));
		w.view.decorator = d = FlowLayout( w.view.bounds, 0@0, 0@0 );
		w.front;

		Button(w.view, 80@40)
		.states_([["start / reset", Color.black, Color.grey]])
		.action_({~start_loop.value});

		Button(w.view, 100@40)
		.states_([["record on", Color.black, Color.grey], ["record off", Color.red, Color.grey]])
		.action_({ |bt|
			//~b.value.postln;
			~loop_s.set(\recRun, bt.value); });

		d.nextLine;
		StaticText(w, 80@40).string_("start: ");
		Slider(w.view, (720-80)@40)
		.action_({|sl| ~set_loop_start.value(sl.value * ~buf.duration); });

		d.nextLine;
		StaticText(w, 80@40).string_("duration: ");
		Slider(w.view, (720-80)@40)
		.action_({|sl| ~set_loop_dur.value(sl.value * ~buf.duration / 8.0); });

		d.nextLine;
		StaticText(w, 80@40).string_("rec level: ");
		Slider(w.view, (720-80)@40)
		.action_({|sl| ~loop_s.set(\rec, sl.value); });

		d.nextLine;
		StaticText(w, 80@40).string_("pre level: ");
		Slider(w.view, (720-80)@40)
		.action_({|sl| ~loop_s.set(\pre, sl.value); });

		d.nextLine;
		StaticText(w, 80@40).string_("rec-fade level: ");
		Slider(w.view, (720-80)@40)
		.action_({|sl| ~loop_s.set(\fadeRec, sl.value); sl.value.postln;});

		d.nextLine;
		StaticText(w, 80@40).string_("pre-fade level: ");
		Slider(w.view, (720-80)@40)
		.action_({|sl| ~loop_s.set(\fadePre, sl.value); sl.value.postln;});

		d.nextLine;
		StaticText(w, 80@40).string_("fade time: ");
		Slider(w.view, (720-80)@40)
		.action_({|sl| ~fade_dur = sl.value; ~loop_s.set(\fade, sl.value); sl.value.postln;});


		d.nextLine;
		StaticText(w, 80@40).string_("rate: ");
		Slider(w.view, (720-80)@40)
		.action_({|sl|
			var x, r;
			x = sl.value.linlin(0, 1, -1, 1);
			r = x * 4.0;
			~loop_s.set(\rate, r);
			r.postln;
		});

	}.defer;

}.play; };
