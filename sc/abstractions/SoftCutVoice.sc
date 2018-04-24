// looped, varispeed, crossfaded playback/recording/overdubbing
// single voice (monophonic)

// light wrapper around synth, busses, some methods

SoftCutVoice {

	var phase_audio_b; // phase output bus (audio rate)
	var <phase_b; // phase output bus (control rate)
	var <loop_b;  // trigger output on loop (audio, single sample)
	var <reset_b; // touch to reset to start position
	var <phase_kr_s; // synth to map audio-rate phase to control rate

	var <syn; // the main synth
	var <buf; // the current buffer

	*new { arg srv, tgt, buf, in, out;
		^super.new.init(srv, tgt, buf, in, out);
	}


	*initClass {

		StartUp.add {
			postln("softcutvoice startup");
			CroneDefs.add(
				// looped, crossfaded , synchronized playback and record
				SynthDef.new(\soft_cut_voice, {
					arg buf, in, out, gate=1,
					phase_out= -1, trig_out= -1, trig_in = -1,
					phase_att_in, phase_att_bypass=0.0,// phase attenuation input
					amp=0.2, rec=0.0, pre=0.0,
					rate=1, ratelag=0.1,
					start=0, end=1, pos=0, fade=0.1, loop=1,
					fadeRec=1.0, fadePre = 1.0, recRun=0, offset=0,
					preLag=0.001, recLag=0.001, envTimeScale = 0.001, done=0;

					var snd, phase, tr;
					var brs;
					var cutfade;
					var trig;
					var sin;
					var aenv;
					var att; // attenuation from input phase

					/// TODO: add an input for arbitrary record head.
					/// this should allow for a crossfade when heads cross
					/// using abs(distance between heads) as xfade envelope

					brs = BufRateScale.kr(buf);

					trig = InTrig.kr(trig_in);
					sin = In.ar(in);

					pre = Lag.kr(pre, preLag);
					rec = Lag.kr(rec, recLag);

					rate = Lag.kr(rate, ratelag);

					cutfade =  SoftCutHead.ar(buf, sin, trig,
						rate * brs, start, end, pos, fade, loop,
						rec, pre, fadeRec, fadePre, recRun, offset);

					phase = cutfade[0];
					tr = cutfade[1];
					snd = cutfade[2];

					aenv = EnvGen.ar(Env.asr(1, 1, 1), gate,
						timeScale:envTimeScale, doneAction:done);

					att = (abs(phase - In.ar(phase_att_in)) * 0.01).min(1.0);
					att = att.max(phase_att_bypass);

					Out.ar(out, ( snd * amp * aenv * att));
					// NB: phase output is audio rate.
					Out.ar(phase_out, phase);
					// NB: this is an _audio_ rate trigger;
					// it stays high for only one sample
					// .kr ugens that read once per audio block will miss it
					Out.ar(trig_out, tr);
				})
			);

			// CroneDefs.add(
			// 	// triggered overdub-recording from arbitrary position
			// 	SynthDef(\rec_dub_trig_gate, {
			// 		arg buf, in, gate=0, done=0,
			// 		rate=1, start=0.0, end=1.0, loop=0,
			// 		rec=1, pre=0, fade=0.01;
			//
			// 		var sr, brs,
			// 		sin, sin_phase,
			// 		phase, wr, trig,
			// 		env_pre, env_rec;
			//
			// 		sr = SampleRate.ir;
			// 		brs = BufRateScale.kr(buf); // NB: BfWr and BufWrPre are non-interpolating...
			// 		env_rec = EnvGen.ar(Env.asr(fade, 1, fade), gate, doneAction:done) * rec;
			// 		env_pre = (pre * env_rec).max(1-env_rec); // soft in/out
			//
			// 		sin = In.ar(in);
			// 		phase = Phasor.ar(gate, rate * brs, start*sr, end*sr, start);
			// 		///// TODO: additional output for phase (see above)
			// 		wr = BufWrPre.ar(sin * env_rec, buf, phase, env_pre);
			// 	})
			// );
		}
	}

	init {
		arg server, target, buf_, in, out;
		buf = buf_;
		reset_b = Bus.control(server);
		phase_audio_b = Bus.audio(server);
		phase_b = Bus.control(server);
		loop_b = Bus.audio(server);

		syn = Synth.new(\soft_cut_voice, [ \buf, buf, \in, in, \out, out, \done, 0,
			\trig_in, reset_b.index, \trig_out, loop_b.index, \phase_out, phase_audio_b.index
		], target);
		phase_kr_s = { Out.kr(phase_b.index, A2K.kr(In.ar(phase_audio_b.index)))}
		.play(target: syn, addAction:\addAfter);

	}

	free {
		phase_audio_b.free;
		phase_b.free;
		loop_b.free;
		reset_b.free;
		phase_kr_s.free;
		syn.free;
	}

	start { syn.set(\gate, 1); syn.run(true); this.reset; }
	
	stop { syn.set(\gate, 0); } // will pause when done
	
	// reset { reset_b.setSynchronous(1); }
	// hm....
	reset { reset_b.set(1); }

	buf_ { arg bf; buf = bf; syn.set(\buf, buf); }

}