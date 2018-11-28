// CroneEngine_Passersby
// West coast style mono synth with complex waveform generation, basic FM and a lowpass gate.
// v1.0.0 Mark Eats

Engine_Passersby : CroneEngine {

	var synthVoice;
	var reverb;
	var lfos;
	var fxBus;
	var lfosBus;
	var numLfoDests = 8;
	var numLfoOuts = 2;
	var lfoDests;
	var poll;
	var waveShapeModPoll, waveFoldsModPoll, fm1AmountModPoll, fm2AmountModPoll, lpgPeakMulModPoll, lpgDecayModPoll, reverbMixModPoll;

	*new { arg context, doneCallback;
		^super.new(context, doneCallback);
	}

	alloc {

		lfoDests = Array.fill(numLfoOuts, 0);

		fxBus = Bus.audio(server: context.server, numChannels: 1);
		lfosBus = Bus.control(server: context.server, numChannels: numLfoDests);

		// Synth voice
		synthVoice = SynthDef(\passersbySynth, {

			arg out, lfosIn, t_gate, freq = 220, pitchBendRatio = 1.0, vel = 0.7,
			waveShape = 0.0, waveFolds = 0.0, fm1Amount = 0.0, fm2Amount = 0.0, lpgPeak = 10000, lpgDecay = 2.0, amp = 1.0;

			var i_nyquist = SampleRate.ir * 0.5, signal, sinOsc, triOsc, additiveOsc, additiveWaveShape = 0, phase,
			fm1Ratio = 0.66, fm2Ratio = 3.3, mod1, mod2, mod1Index, mod2Index, mod1Freq, mod2Freq, modMult,
			releaseTime, filterEnvVel, filterEnvLow, lpgEnvGen;

			// LFO ins
			freq = (freq * In.kr(bus: lfosIn, numChannels: numLfoDests)[0]).clip(0, i_nyquist);
			waveShape = (waveShape + In.kr(bus: lfosIn, numChannels: numLfoDests)[1]).clip(0, 1);
			waveFolds = (waveFolds + In.kr(bus: lfosIn, numChannels: numLfoDests)[2]).clip(0, 3);
			fm1Amount = (fm1Amount + In.kr(bus: lfosIn, numChannels: numLfoDests)[3]).clip(0, 1);
			fm2Amount = (fm2Amount + In.kr(bus: lfosIn, numChannels: numLfoDests)[4]).clip(0, 1);
			lpgPeak = (lpgPeak * In.kr(bus: lfosIn, numChannels: numLfoDests)[5]).clip(100, 10000);
			lpgDecay = (lpgDecay + In.kr(bus: lfosIn, numChannels: numLfoDests)[6]).clip(0.1, 8.0);

			// Lag inputs
			freq = Lag.kr(in: freq * pitchBendRatio, lagTime: 0.01);
			vel = Lag.kr(in: vel, lagTime: 0.01);
			waveShape = Lag.kr(in: waveShape, lagTime: 0.2);
			waveFolds = Lag.kr(in: waveFolds, lagTime: 0.1);
			fm1Amount = Lag.kr(in: fm1Amount.squared, lagTime: 0.2);
			fm2Amount = Lag.kr(in: fm2Amount.squared, lagTime: 0.2);
			lpgPeak = Lag.kr(in: lpgPeak, lagTime: 0.1);
			lpgDecay = Lag.kr(in: lpgDecay, lagTime: 0.1);

			// Oscillator

			// FM modulators
			mod1Index = fm1Amount * 22;
			mod1Freq = freq * fm1Ratio * LFNoise2.kr(freq: 0.1, mul: 0.001, add: 1);
			mod1 = SinOsc.ar(freq: mod1Freq, phase: 0, mul: mod1Index * mod1Freq, add: 0);
			mod2Index = fm2Amount * 12;
			mod2Freq = freq * fm2Ratio * LFNoise2.kr(freq: 0.1, mul: 0.005, add: 1);
			mod2 = SinOsc.ar(freq: mod2Freq, phase: 0, mul: mod2Index * mod2Freq, add: 0);

			// Carrier waves

			sinOsc = SinOsc.ar(freq: freq + mod1 + mod2, phase: 0, mul: 0.5);
			triOsc = VarSaw.ar(freq: freq + mod1 + mod2, iphase: 0, width: 0.5, mul: 0.5);

			additiveWaveShape = waveShape.linlin(0.666666, 1, 0, 1);
			phase = LFSaw.ar(freq: freq + mod1 + mod2, iphase: 1, mul: pi, add: pi);

			modMult = max(fm1Ratio * (mod1Index.linlin(0, 0.3, 0, 1) + mod1Index), fm2Ratio * (mod2Index.linlin(0, 0.3, 0, 1) + mod2Index));

			// Makes square and saw
			additiveOsc = Mix.fill(32, {
				arg index;
				var harmonic, harmonicFreq, harmonicCutoff, attenuation;

				harmonic = index + 1;
				harmonicFreq = freq * harmonic;
				harmonicCutoff = i_nyquist - harmonicFreq;

				// Attenuate harmonics that will go over nyquist once FM is applied
				attenuation = Select.kr(index, [ 1, // Save the fundamental
					(harmonicCutoff - (harmonicFreq * 0.25) - harmonicFreq).expexp(0.000001, harmonicFreq * 0.5, 0.000001, 1) ]);

				(sin(phase * harmonic % 2pi) / harmonic) * attenuation * (harmonic % 2 + additiveWaveShape).min(1);
			}) * 0.25;

			signal = LinSelectX.ar(waveShape * 4, [sinOsc, triOsc, additiveOsc]); // Mix

			// Hack away some aliasing
			signal = LPF.ar(in: signal, freq: 12000);

			// Fold
			signal = Fold.ar(in: signal * (1 + (waveFolds * 2)), lo: -0.5, hi: 0.5);

			// Hack away some more
			signal = LPF.ar(in: signal, freq: 12000);

			// Noise
			signal = signal + PinkNoise.ar(mul: 0.0065);


			// LPG

			// Filter env
			filterEnvVel = vel.linlin(0, 1, 0.5, 1);
			filterEnvLow = (lpgPeak * filterEnvVel).min(300);
			lpgEnvGen = EnvGen.ar(envelope: Env.new(levels: [filterEnvLow, lpgPeak * filterEnvVel, filterEnvLow], times: [0.003, lpgDecay], curve: [4, -20]), gate: t_gate);

			signal = RLPF.ar(in: signal, freq: lpgEnvGen, rq: 0.9);

			// Amp env
			signal = signal * EnvGen.ar(envelope: Env.new(levels: [0, vel.linexp(0, 1, 0.2, 1), 0], times: [0.002, lpgDecay], curve: [4, -10]), gate: t_gate);


			// Saturation amp
			signal = tanh(signal * 1.8) * amp;

			Out.ar(bus: out, channelsArray: signal.dup);

		}).play(target:context.xg, args: [\out, fxBus, \lfosIn, lfosBus]);


		// Very approx spring reverb
		reverb = SynthDef(\reverb, {

			arg in, out, lfosIn, mix = 0.0;
			var dry, preProcess, springReso, wet, predelay = 0.015;

			mix = (mix + In.kr(bus: lfosIn, numChannels: numLfoDests)[7]).clip(0, 1);
			mix = Lag.kr(in: mix, lagTime: 0.2);

			dry = In.ar(bus: in, numChannels: 1);

			preProcess = tanh(BHiShelf.ar(in: dry, freq: 1000, rs: 1, db: -6, mul: 1.5, add: 0)); // Darken and saturate
			preProcess = DelayN.ar(in: preProcess, maxdelaytime: predelay, delaytime: predelay);
			springReso = Klank.ar(specificationsArrayRef: `[[508, 270, 1153], [0.15, 0.25, 0.1], [1, 1.2, 1.4]], input: preProcess);
			springReso = Limiter.ar(springReso).dup;
			preProcess = preProcess * 0.55; // FreeVerb doesn't like a loud signal
			wet = tanh(FreeVerb2.ar(in: preProcess, in2: preProcess, mix: 1, room: 0.7, damp: 0.35, mul: 1.8));
			wet = (wet * 0.935) + (springReso * 0.065);

			Out.ar(bus: out, channelsArray: (dry.dup * (1.0 - mix)) + (wet * mix));

		}).play(target:context.xg, args: [\in, fxBus, \out, context.out_b, \lfosIn, lfosBus], addAction: \addToTail);


		// LFOs
		lfos = SynthDef(\lfos, {

			arg out, lfoFreq = 0.5, lfoAmount = 0.5, lfoDests = #[0, 0], drift = 0.0;
			var i_driftRate = 0.15, i_outIds = #[0, 1, 2, 3, 4, 5, 6, 7], outArray, lfo, lfoOut1, lfoOut2;

			outArray = Array.fill(numLfoDests, 0);

			// LFO
			lfo = LFTri.ar(freq: lfoFreq, mul: lfoAmount);
			outArray.do({
				arg val, i;
				lfoDests.do({
					arg dest, j;
					outArray[i] = outArray[i] + (lfo * BinaryOpUGen('==', dest - 1, i));
				});
			});

			// Drift and scale
			outArray[0] = (outArray[0] * 18).midiratio; // Freq ratio
			outArray[1] = outArray[1] + LFNoise1.kr(freq: i_driftRate, mul: drift); // Wave Shape
			outArray[2] = (outArray[2] + LFNoise1.kr(freq: i_driftRate, mul: drift)) * 2; // Wave Folds
			outArray[3] = (outArray[3] + LFNoise1.kr(freq: i_driftRate, mul: drift)) * 0.5; // FM1 Amount
			outArray[4] = (outArray[4] + LFNoise1.kr(freq: i_driftRate, mul: drift)) * 0.5; // FM2 Amount
			outArray[5] = ((outArray[5] + LFNoise1.kr(freq: i_driftRate, mul: drift)) * 24).midiratio; // LPG Peak multiplier
			outArray[6] = (outArray[6] + LFNoise1.kr(freq: i_driftRate, mul: drift)) * 2.2; // LPG Decay
			outArray[7] = outArray[7] + LFNoise1.kr(freq: i_driftRate, mul: drift); // Reverb Mix

			SendTrig.kr(in: Impulse.kr(15), id: i_outIds, value: outArray);
			Out.kr(bus: out, channelsArray: outArray);

		}).play(target:context.xg, args: [\out, lfosBus]);


		// Receive messages from server
		OSCFunc({
			arg msg;
			var index = msg[2];
			var value = msg[3];
			switch(index,
				1, { waveShapeModPoll.update(value) },
				2, { waveFoldsModPoll.update(value) },
				3, { fm1AmountModPoll.update(value) },
				4, { fm2AmountModPoll.update(value) },
				5, { lpgPeakMulModPoll.update(value) },
				6, { lpgDecayModPoll.update(value) },
				7, { reverbMixModPoll.update(value) }
			);
		}, path: '/tr', srcID: context.server.addr);

		// Polls
		waveShapeModPoll = this.addPoll(name: "waveShapeMod", periodic: false);
		waveFoldsModPoll = this.addPoll(name: "waveFoldsMod", periodic: false);
		fm1AmountModPoll = this.addPoll(name: "fm1AmountMod", periodic: false);
		fm2AmountModPoll = this.addPoll(name: "fm2AmountMod", periodic: false);
		lpgPeakMulModPoll = this.addPoll(name: "lpgPeakMulMod", periodic: false);
		lpgDecayModPoll = this.addPoll(name: "lpgDecayMod", periodic: false);
		reverbMixModPoll = this.addPoll(name: "reverbMixMod", periodic: false);


		// Commands

		// noteOn(id, freq, vel)
		this.addCommand("noteOn", "ifi", { arg msg;
			synthVoice.set(\freq, msg[2], \vel, msg[3], \t_gate, 1);
		});

		// noteOff(id)
		this.addCommand("noteOff", "i", { arg msg;

		});

		// pitchBend(ratio)
		this.addCommand("pitchBend", "f", { arg msg;
			synthVoice.set(\pitchBendRatio, msg[1]);
		});

		this.addCommand("waveShape", "f", { arg msg;
			synthVoice.set(\waveShape, msg[1]);
		});

		this.addCommand("waveFolds", "f", { arg msg;
			synthVoice.set(\waveFolds, msg[1]);
		});

		this.addCommand("fm1Amount", "f", { arg msg;
			synthVoice.set(\fm1Amount, msg[1]);
		});

		this.addCommand("fm2Amount", "f", { arg msg;
			synthVoice.set(\fm2Amount, msg[1]);
		});

		this.addCommand("lpgPeak", "f", { arg msg;
			synthVoice.set(\lpgPeak, msg[1]);
		});

		this.addCommand("lpgDecay", "f", { arg msg;
			synthVoice.set(\lpgDecay, msg[1]);
		});

		this.addCommand("reverbMix", "f", { arg msg;
			reverb.set(\mix, msg[1]);
		});

		this.addCommand("lfoFreq", "f", { arg msg;
			lfos.set(\lfoFreq, msg[1]);
		});

		this.addCommand("lfoAmount", "f", { arg msg;
			lfos.set(\lfoAmount, msg[1]);
		});

		this.addCommand("lfoDest", "ii", { arg msg;
			lfoDests[msg[1].clip(0, numLfoOuts - 1)] = msg[2];
			lfos.set(\lfoDests, lfoDests);
		});

		this.addCommand("drift", "f", { arg msg;
			lfos.set(\drift, msg[1]);
		});

	}

	free {
		synthVoice.free;
		reverb.free;
		lfos.free;
	}
}
