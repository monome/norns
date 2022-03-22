// a subtractive polysynth engine

Engine_PolySub : CroneEngine {

	classvar <polyDef;
	classvar <paramDefaults;
	classvar <maxNumVoices;

	var <ctlBus; // collection of control busses
	var <mixBus; // audio bus for mixing synth voices
	var <gr; // parent group for voice nodes
	var <voices; // collection of voice nodes

	*initClass {
		maxNumVoices = 16;
		StartUp.add {
			// a decently versatile subtractive synth voice
			polyDef = SynthDef.new(\polySub, {
				arg out, gate=1, hz, level=0.2, // the basics
				shape=0.0, // base waveshape selection
				timbre=0.5, // modulation of waveshape
				sub=0.4, // sub-octave sine level
				noise = 0.0, // pink noise level (before filter)
				cut=8.0, // RLPF cutoff frequency as ratio of fundamental
				// amplitude envelope params
				ampAtk=0.05, ampDec=0.1, ampSus=1.0, ampRel=1.0, ampCurve= -1.0,
				// filter envelope params
				cutAtk=0.0, cutDec=0.0, cutSus=1.0, cutRel=1.0,
				cutCurve = -1.0, cutEnvAmt=0.0,
				fgain=0.0, // filter gain (moogFF model)
				detune=0, // linear frequency detuning between channels
				width=0.5,// stereo width
				hzLag = 0.1;

				var osc1, osc2, snd, freq, del, aenv, fenv;

				// TODO: could add control over these lag times if you wanna get crazy
				detune = Lag.kr(detune);
				shape = Lag.kr(shape);
				timbre = Lag.kr(timbre);
				fgain = Lag.kr(fgain.min(4.0));
				cut = Lag.kr(cut);
				width = Lag.kr(width);

				detune = detune / 2;
				hz = Lag.kr(hz, hzLag);
				freq = [hz + detune, hz - detune];
				osc1 = VarSaw.ar(freq:freq, width:timbre);
				osc2 = Pulse.ar(freq:freq, width:timbre);
				// TODO: could add more oscillator types


				// FIXME: probably a better way to do this channel selection
				snd = [SelectX.ar(shape, [osc1[0], osc2[0]]), SelectX.ar(shape, [osc1[1], osc2[1]])];
				snd = snd + ((SinOsc.ar(hz / 2) * sub).dup);
				aenv = EnvGen.ar(
					Env.adsr(ampAtk, ampDec, ampSus, ampRel, 1.0, ampCurve),
					gate, doneAction:2);

				fenv = EnvGen.ar(Env.adsr(cutAtk, cutDec, cutSus, cutRel), gate);

				cut = SelectX.kr(cutEnvAmt, [cut, cut * fenv]);
				cut = (cut * hz).min(SampleRate.ir * 0.5 - 1);

				snd = SelectX.ar(noise, [snd, [PinkNoise.ar, PinkNoise.ar]]);
				snd = MoogFF.ar(snd, cut, fgain) * aenv;

				Out.ar(out, level * SelectX.ar(width, [Mix.new(snd).dup, snd]));
			});

			CroneDefs.add(polyDef);

			paramDefaults = Dictionary.with(
				\level -> -12.dbamp,
				\shape -> 0.0,
				\timbre -> 0.5,
				\noise  ->  0.0,
				\cut -> 8.0,
				\ampAtk -> 0.05, \ampDec -> 0.1, \ampSus -> 1.0, \ampRel -> 1.0, \ampCurve ->  -1.0,
				\cutAtk -> 0.0, \cutDec -> 0.0, \cutSus -> 1.0, \cutRel -> 1.0,
				\cutCurve  ->  -1.0, \cutEnvAmt -> 0.0,
				\fgain -> 0.0,
				\detune -> 0,
				\width -> 0.5,
				\hzLag -> 0.1
			);

		} // Startup
	} // initClass

	*new { arg context, callback;
		^super.new(context, callback);
	}

	alloc {
		gr = ParGroup.new(context.xg);

		voices = Dictionary.new;
		ctlBus = Dictionary.new;
		polyDef.allControlNames.do({ arg ctl;
			var name = ctl.name;
			postln("control name: " ++ name);
			if((name != \gate) && (name != \hz) && (name != \out), {
				ctlBus.add(name -> Bus.control(context.server));
				ctlBus[name].set(paramDefaults[name]);
			});
		});

		ctlBus.postln;

		ctlBus[\level].setSynchronous( 0.2 );


		//--------------
		//--- voice control, all are indexed by arbitarry ID number
		// (voice allocation should be performed by caller)

		// start a new voice
		this.addCommand(\start, "if", { arg msg;
			this.addVoice(msg[1], msg[2], true);
		});


		// same as start, but don't map control busses, just copy their current values
		this.addCommand(\solo, "if", { arg msg;
			this.addVoice(msg[1], msg[2], false);
		});


		// stop a voice
		this.addCommand(\stop, "i", { arg msg;
			this.removeVoice(msg[1]);
		});

		// free all synths
		this.addCommand(\stopAll, "", {
			gr.set(\gate, 0);
			voices.clear;
		});

		// generate commands to set each control bus
		ctlBus.keys.do({ arg name;
			this.addCommand(name, "f", { arg msg; ctlBus[name].setSynchronous(msg[1]); });
		});

		postln("polysub: performing init callback");
	}

	addVoice { arg id, hz, map=true;
		var params = List.with(\out, context.out_b.index, \hz, hz);
		var numVoices = voices.size;

		if(voices[id].notNil, {
			voices[id].set(\gate, 1);
			voices[id].set(\hz, hz);
		}, {
			if(numVoices < maxNumVoices, {
				ctlBus.keys.do({ arg name;
					params.add(name);
					params.add(ctlBus[name].getSynchronous);
				});

				voices.add(id -> Synth.new(\polySub, params, gr));
				NodeWatcher.register(voices[id]);
				voices[id].onFree({
					voices.removeAt(id);
				});

				if(map, {
					ctlBus.keys.do({ arg name;
						voices[id].map(name, ctlBus[name]);
					});
				});
			});
		});
	}

	removeVoice { arg id;
		if(true, {
			voices[id].set(\gate, 0);
		});
	}

	free {
		gr.free;
		ctlBus.do({ arg bus, i; bus.free; });
		mixBus.do({ arg bus, i; bus.free; });
		
	}

} // class
