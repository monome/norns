// a subtractive polysynth engine

Engine_PolySub : CroneEngine {

	classvar polyDef, compVerbDef;
	classvar paramDefaults;

	var ctx; // audio context
	var ctlBus; // collection of control busses
	var mixBus; // audio bus for mixing synth voices
	var gr; // parent group for voice nodes
	var voices; // collection of voice nodes
	var compVerbSyn; // compression/reverb/output synth

	*initClass {
		StartUp.add {
			// a decently versatile subtractive synth voice
			polyDef = SynthDef.new(\polysub, {
				arg out, gate=1, hz, level=0.2, // the basics
				shape=0.0, // base waveshape selection
				timbre=0.5, // modulation of waveshape
				noise = 0.0, // pink noise level (before filter)
				cut=8.0, // RLPF cutoff frequency as ratio of fundamental
				// amplitude envelope params
				ampAtk=0.05, ampDec=0.1, ampSus=1.0, ampRel=1.0, ampCurve= -1.0,
				// filter envelope params
				cutAtk=0.0, cutDec=0.0, cutSus=1.0, cutRel=1.0,
				cutCurve = -1.0, cutEnvAmt=0.0,
				fgain=0.0, // filter gain (moogFF model)
				detune=0, // linear frequency detuning between channels
				delTime=0.2, delMix=0.0, delFb=0.0, // basic delay parameters
				delSpread=0.0, // delay offset in seconds between L/R channels
				width=0.5; // stereo width

				var osc1, osc2, snd, freq, del, aenv, fenv;

				// TODO: could add control over these lag times if you wanna get crazy
				detune = Lag.kr(detune);
				shape = Lag.kr(shape);
				timbre = Lag.kr(timbre);
				fgain = Lag.kr(fgain);
				cut = Lag.kr(cut);
				delTime = Lag.kr(delTime);
				delSpread = Lag.kr(delSpread);
				width = Lag.kr(width);

				detune = detune / 2;
				freq = [hz + detune, hz - detune];
				osc1 = VarSaw.ar(freq:freq, width:timbre);
				osc2 = Pulse.ar(freq:freq, width:timbre);
				// TODO: could add more oscillator types


				// FIXME: probably a better way to do this channel selection
				snd = [SelectX.ar(shape, [osc1[0], osc2[0]]), SelectX.ar(shape, [osc1[1], osc2[1]])];

				aenv = EnvGen.ar(
					Env.adsr(ampAtk, ampDec, ampSus, ampRel, 1.0, ampCurve),
					gate);

				fenv = EnvGen.ar(Env.adsr(cutAtk, cutDec, cutSus, cutRel), gate);

				cut = SelectX.kr(cutEnvAmt, [cut, cut * fenv]);
				cut = cut * hz;


				snd = SelectX.ar(noise, [snd, [PinkNoise.ar, PinkNoise.ar]]);
				snd = MoogFF.ar(snd, cut, fgain) * aenv;
				del = DelayC.ar(snd, 0.5, [(delTime + delSpread).max(0), (delTime - delSpread).max(0)]);
				LocalOut.ar(del);
				del = del + (delFb * LocalIn.ar(2));
				snd = SelectX.ar(delMix, [snd, del]);
				FreeSelf.kr(DetectSilence.ar(snd));
				Out.ar(out, level * SelectX.ar(width, [Mix.new(snd).dup, snd]));
			});

			// a basic (hard-knee) stereo compressor and reverb
			compVerbDef = SynthDef.new(\compVerb, {
				arg in, out,
				thresh = 0.6,
				atk = 0.01,
				rel = 0.1,
				slope = 8.0,
				compMix = 1.0,
				room=0.5, damp=0.0,
				verbMix=0.0;

				var snd = In.ar(in, 2);
				snd = SelectX.ar(compMix, [snd, Compander.ar(snd, snd, thresh, slopeAbove:slope, clampTime:atk, relaxTime:rel)]);
				snd = SelectX.ar(verbMix, [snd, FreeVerb2.ar(snd[0], snd[1], mix:1.0, room:room, damp:damp)]);
				Out.ar(out, snd);
			});

			CroneDefs.add(polyDef);
			CroneDefs.add(compVerbDef);

			//// FIXME: probably a better way... ehh.
			paramDefaults = Dictionary.with(
				\shape -> 0.0,
				\timbre -> 0.5,
				\noise  ->  0.0,
				\cut -> 8.0,
				\ampAtk -> 0.05, \ampDec -> 0.1, \ampSus -> 1.0, \ampRel -> 1.0, \ampCurve ->  -1.0,
				\cutAtk -> 0.0, \cutDec -> 0.0, \cutSus -> 1.0, \cutRel -> 1.0,
				\cutCurve  ->  -1.0, \cutEnvAmt -> 0.0,
				\fgain -> 0.0,
				\detune -> 0,
				\delTime -> 0.2, \delMix -> 0.0, \delFb -> 0.0,
				\delSpread -> 0.0,
				\width -> 0.5
			);

		} // Startup
	} // initClass

	*new { arg context; ^super.new.init(context).initSub(context); }

	initSub  { arg context;
		context.postln;
		ctx = context;
		gr = Group.new(ctx.xg);

		voices = Dictionary.new;
		ctlBus = Dictionary.new;
		polyDef.allControlNames.do({ arg ctl;
			var name = ctl.name;
			postln("control name: " ++ name);
			if((name != \gate) && (name != \hz) && (name != \out), {
				ctlBus.add(name -> Bus.control(ctx.server));
				ctlBus[name].set(paramDefaults[name]);
			});
		});

		ctlBus[\level] = 0.2;

		// mix bus for all synth outputs
		mixBus =  Bus.audio(ctx.server, 2);


		// throw a little crummy comp and reverb on there, send to the context output
		compVerbSyn = Synth.new(\compVerb, [\in, mixBus, \out, ctx.out_b], gr, \addAfter);


		this.addCommand(\start, "if", { arg msg;
			// FIXME: should have a NodeWatcher or something to limit number of synths
			voices.add(msg[1]-> Synth.new(\polySub, [\out, mixBus, \hz, msg[2]], gr));
			ctlBus.keys.do({ arg param; voices[msg[1]].map(param, ctlBus[param]); });
		});

		this.addCommand(\stop, "i", { arg msg;
			var syn = voices[msg[1]];
			if(syn.notNil, { syn.set(\gate, 0); });
		});

		// free all synths
		this.addCommand(\freeAll, "", { gr.set(\gate, 0); });

		// generate commands to set each control bus
		ctlBus.keys.do({ arg name;
			this.addCommand(name, "f", { arg msg; ctlBus[name].setSynchronous(msg[1]); });
		});


	} // init


	free {
		gr.free;
		compVerbSyn.free;
		mixBus.free;
		ctlBus.do({ arg bus, i; bus.free; });
		super.free;
	}

} // class