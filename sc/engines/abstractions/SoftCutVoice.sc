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
					preLag=0.001, recLag=0.001, envTimeScale = 0.001;

					var snd, phase, tr;
					var brs;
					var cutfade;
					var trig;
					var sin;
					var aenv;
					var att; // attenuation from input phase

					brs = BufRateScale.kr(buf);

					trig = InTrig.kr(trig_in);
					sin = In.ar(in);

					rec = Lag.ar(K2A.ar(rec), recLag);
					pre = Lag.ar(K2A.ar(pre), preLag);
					rate = Lag.ar(K2A.ar(rate), ratelag);

					cutfade =  SoftCutHead.ar(buf,
						sin, rec, pre, rate * brs,
						pos, trig,  start, end, fade, loop,
						fadeRec, fadePre, recRun, offset);

					phase = cutfade[0];
					tr = cutfade[1];
					snd = cutfade[2];

					aenv = EnvGen.kr(Env.asr(1, 1, 1), gate,
						timeScale:envTimeScale);

					att = (abs(phase - In.ar(phase_att_in)) * 0.01).min(1.0);
					att = att.max(phase_att_bypass);

					// Out.ar(out, ( snd * amp * aenv * att));
					Out.ar(out, ( snd * amp * aenv));
					Out.kr(phase_out, Gate.kr(A2K.kr(phase) * SampleDur.ir, gate));
					Out.kr(trig_out, A2K.kr(tr));
				})
			);
		}
	}

	init {
		arg server, target, buf_, in, out;
		buf = buf_;
		reset_b = Bus.control(server);
		phase_b = Bus.control(server); 
		loop_b = Bus.control(server);

		syn = Synth.new(\soft_cut_voice, [ \buf, buf, \in, in, \out, out,
			\trig_in, reset_b.index, \trig_out, loop_b.index, \phase_out, phase_b.index
		], target);


		/*
		// FIXME: we aren't using AR phase at the moment
		phase_kr_s = { arg gate = 1;
			var phase = A2K.kr(In.ar(phase_audio_b.index));
			phase = Gate.kr(phase, gate);
			Out.kr(phase_b.index, phase);
			}.play(target: syn, addAction:\addAfter);
		*/

	}

	free {
		phase_audio_b.free;
		phase_b.free;
		loop_b.free;
		reset_b.free;
		phase_kr_s.free;
		syn.free;
	}

	start { syn.set(\gate, 1); } //syn.run(true); this.reset; phase_kr_s.set(\gate, 1); }
	stop { syn.set(\gate, 0); } //phase_kr_s.set(\gate, 0); }
	// hm....
	reset { reset_b.set(1); }
	buf_ { arg bf; buf = bf; syn.set(\buf, buf); }

}