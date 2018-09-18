// utility class to load synthdefs required by crone engines.
// engines could of course define their own defs, this is for shared functionality.
CroneDefs {

	classvar defs;

	*add  { arg def;
		if(defs.isNil,  {
			defs = List.new;
		}, {
			defs.add(def);
		});
	}

	*initClass {
		if(defs.isNil, { defs = List.new;
		});

		// single read head with fade in/out trigger
		defs.add(
			SynthDef.new(\play_fade, {
				arg out=0, buf=0,
				start=0, trig=0, rate=1.0, loop=0,
				gate, fade_time=0.2, fade_shape=0.0,
				mute=0, level=1.0;
				var snd, aenv, amp, phase;
				phase = Sweep.ar(InTrig.kr(trig), BufSampleRate.kr(buf) * rate);
				snd =  BufRd.ar(1, buf, phase + start, loop:loop);
				aenv = Env.asr(fade_time, 1.0, fade_time, fade_shape);
				amp = EnvGen.kr(aenv, gate);
				amp = amp * Lag.kr(level * (1 - mute));
				Out.ar(out, (snd * amp));
			})
		);

		// mono patch with smoothing
		defs.add(
			SynthDef.new(\patch_mono, {
				arg in, out, level=1.0, lag=0.01;
				var ampenv = Lag.kr(level, lag);
				Out.ar(out, In.ar(in) * ampenv);
			})
		);

		// mono patch with smoothing and feedback
		// (InFeedback introduces 1 audio block of delay)
		defs.add(
			SynthDef.new(\patch_mono_fb, {
				arg in, out, level=1.0, lag=0.01;
				var ampenv = Lag.kr(level, lag);
				Out.ar(out, InFeedback.ar(in) * ampenv);
			})
		);

		// stereo patch with smoothing
		defs.add(
			SynthDef.new(\patch_stereo, {
				arg in, out, level=1.0, lag=0.01;
				var ampenv = Lag.kr(level, lag);
				Out.ar(out, In.ar(in, 2) * ampenv);
			})
		);

		// mono->stereo patch with smoothing and pan
		defs.add(
			SynthDef.new(\patch_pan, {
				arg in, out, level=1.0, pan=0, lag=0.01;
				var ampenv = Lag.kr(level, lag);
				var panenv = Lag.kr(pan, lag);
				Out.ar(out, Pan2.ar(In.ar(in) * ampenv, pan));
			})
		);

		// record with some level smoothing
		defs.add(
			SynthDef.new(\rec_smooth, {
				arg buf, in, offset=0, rec=1, pre=0, lag=0.01,
				run=1, loop=0, trig=0, done=0;
				var ins, pres, recs;
				ins = In.ar(in);
				pres = Lag.kr(pre, lag);
				recs = Lag.kr(rec, lag);
				RecordBuf.ar(ins, buf,
					recLevel:rec, preLevel:pre,
					offset:offset, trigger: InTrig.kr(trig),
					loop:0, doneAction: done);
			})
		);

		// raw mono adc input
		defs.add(
			SynthDef.new(\adc, {
				arg in, out, level=1.0, lag=0.01;
				var ampenv = Lag.kr(level, lag);
				Out.ar(out, SoundIn.ar(in) * ampenv)
			})
		);

		// envelope follower (audio input, control output)
		defs.add(
			SynthDef.new(\amp_env, {
				arg in, out, atk=0.01, rel=0.01;
				var absin = abs(A2K.kr(In.ar(in)));
				var amp = LagUD.kr(absin, atk, rel);
				Out.kr(out, amp);
			})
		);

		// pitch follower
		defs.add(
			SynthDef.new(\pitch, {
				arg in, out,
				initFreq = 440.0, minFreq = 30.0, maxFreq = 10000.0,
				execFreq = 50.0, maxBinsPerOctave = 16, median = 1,
				ampThreshold = 0.01, peakThreshold = 0.5, downSample = 2, clar=0;
				// Pitch ugen outputs an array of two values:
				// first value is pitch, second is a clarity value in [0,1]
				// if 'clar' argument is 0 (default) then clarity output is binary
				var pc = Pitch.kr(In.ar(in),
					initFreq , minFreq , maxFreq ,
					execFreq , maxBinsPerOctave , median ,
					ampThreshold , peakThreshold , downSample, clar
				);
				//pc.poll;
				Out.kr(out, pc);
			})
		);
	}


	*sendDefs { arg s;
		postln("CroneDefs: sending defs");
		defs.do({ arg def;
			postln(def.name);
			def.send(s);
		});
	}
}
