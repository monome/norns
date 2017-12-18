s.waitForBoot { Routine {

	~buf = Buffer.read(s, PathName(Document.current.path).pathOnly ++ "vla_test_48.wav");

	s.sync;

	postln("buffer samplerate: " ++ ~buf.sampleRate ++  " ; duration: " ++ ~buf.duration);

	if(~buf.sampleRate != s.sampleRate, {
		postln("WARNING: buffer and server have different sample rates. recording over this buffer may produce unwanted artifacts. ");
	});

	{ s.scope }.defer;


	// looped, crossfaded playback
	SynthDef.new(\cutfadeloop_play, {
		arg buf, out, phase_out= -1, trig_out= -1, trig_in = -1,
		amp=0.2,
		//trig=0,
		rate=1, start=0, end=1, fade=0.1, loop=1;
		var snd, phase, tr;
		var brs;
		var cutfade;
		var trig;

		brs = BufRateScale.kr(buf);

		trig = InTrig.kr(trig_in);

		cutfade =  CutFadeLoop.ar(buf, trig, rate * brs, start, end, fade, loop);
		phase = cutfade[0];
		tr = cutfade[1];
		snd = cutfade[2];


		Out.ar(out, (snd*amp).dup);
		Out.ar(phase_out, phase);
		Out.ar(trig_out, tr);
	}).send(s);


	// triggered recording from arbitrary position
	SynthDef(\cutfade_rec_trig_gate, {
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

	// continuous recording following play head
	SynthDef.new(\cutfade_rec_follow_gate, {
		arg buf, in, in_phase, in_trig, gate=0,
		rec=1, pre=0, fade=0.01, delay_samps=0;

		var sr, bs,
		sin, sin_phase, sin_trig,
		phase, wr, trig,
		env_pre, env_rec;

		sr = SampleRate.ir;
		bs = BufSamples.kr(buf);

		env_rec = EnvGen.ar(Env.asr(fade, 1, fade), gate) * rec;
		env_pre = (pre * env_rec).max(1-env_rec);
	//	[env_rec, env_pre].poll;

		sin = In.ar(in);
		sin_phase = In.ar(in_phase);
		sin_trig= In.ar(in_trig);

		sin_phase.poll;

//		phase = (sin_phase - delay_samps).wrap(0, bs-1);
		phase = sin_phase;
//		phase.poll;
		//		hmmm...
		//		phase = sin_phase;

		// FIXME: use loop trigger and select between two different writers... or something
		// currently, will click on loop...

		wr = BufWrPre.ar(sin * env_rec, buf, phase, env_pre);
	}).send(s);

	s.sync;
	//--- state

	~start_pos = 0.0;
	~end_pos = 1.0;
	~fade_dur = 0.1;
	~play_dur = ~end_pos - ~start_pos;

	~phase_rd_b= Bus.audio(s, 1);
	~trig_rd_b= Bus.audio(s, 1);

	// touching this control bus will trigger playback
	~play_trig_b = Bus.control(s, 1);

	~play_s = Synth.new(\cutfadeloop_play, [
		\buf, ~buf.bufnum,
		\out, Crone.ctx.out_b.index,
		\start, ~start_pos, \end, ~end_pos,
		\phase_out, ~phase_rd_b.index,
		\trig_out, ~trig_rd_b.index,
		\trig_in, ~play_trig_b.index,
		\fade, ~fade_dur
	], s);

	~rec_trig_s = Synth.new(\cutfade_rec_trig_gate, [
		\in, Crone.ctx.in_b[0].index,
		\start, 0,
		\end, ~buf.duration,
		\buf, ~buf.bufnum,
	], s, addAction: \addToTail);

	/*
	~rec_follow_s.free;
	*/
	~rec_follow_s = Synth.new(\cutfade_rec_follow_gate, [
		\in, Crone.ctx.in_b[0].index,
		\in_phase, ~phase_rd_b.index,
		\in_trig, ~trig_rd_b.index,
		\buf, ~buf.bufnum,
	], s, addAction: \addToTail);
	~rec_follow_s.set(\pre, 0.5);

	//--- methods

	~update_end_pos = { arg pos;
		~end_pos = pos.min(~buf.duration - ~fade_dur);
		[~start_pos, ~play_dur, ~end_pos].postln;
		~play_s.set(\end, ~end_pos);
	};

	~update_play_dur = { arg dur;
		dur.postln;
		~play_dur = dur.max(~fade_dur + (10.0 / s.sampleRate));
		~update_end_pos.value(~start_pos + ~play_dur);
	};

	~start_playback = { ~play_trig_b.set(1); };

	~set_play_loop = { arg val; ~play_s.set(\loop, val); };

	~set_rec_trig_gate = { arg val; ~rec_trig_s.set(\gate, val); };

	~set_rec_follow_gate = { arg val; ~rec_follow_s.set(\gate, val); };

	~set_play_start = { arg pos, keepDur=true;
		~start_pos = pos;
		[~start_pos, ~play_dur, ~end_pos].postln;
		~play_s.set(\start, ~start_pos);
		if(keepDur, {
			~update_end_pos.value(~start_pos + ~play_dur);
		});
	};

	~set_play_end = { arg pos; ~update_end_pos.value(pos); };
	~set_play_dur = { arg dur; ~update_play_dur.value(dur); };

	//-- test gui
	{
		w = Window("", Rect(0, 0, 720, 270));
		w.view.decorator = d = FlowLayout( w.view.bounds, 0@0, 0@0 );
		w.front;

		Button(w.view, 80@40)
		.states_([["play", Color.black, Color.grey]])
		.action_({~start_playback.value});

		Button(w.view, 100@40)
		.states_([["record trigger", Color.black, Color.grey], ["stop", Color.red, Color.grey]])
		.action_({ |bt| ~set_rec_trig_gate.value(bt.value); });


		Button(w.view, 100@40)
		.states_([["record follow", Color.black, Color.grey], ["stop", Color.red, Color.grey]])
		.action_({ |bt|
			~set_rec_follow_gate.value(bt.value);
		});


		d.nextLine;
		StaticText(w, 80@40).string_("start: ");
		Slider(w.view, (720-80)@40)
		.action_({|sl| ~set_play_start.value(sl.value * ~buf.duration); });

		d.nextLine;
		StaticText(w, 80@40).string_("dur: ");
		Slider(w.view, (720-80)@40)
		.action_({|sl| ~set_play_dur.value(sl.value * ~buf.duration / 8.0); });
	}.defer;

	/*
	~rec_trig_s.set(\pre, 1);
	~rec_trig_s.set(\pre, 0.25);
	~play_s.set(\fade, 0.1);

	~rec_follow_s.set(\pre, 0.5);
	*/


}.play; };
