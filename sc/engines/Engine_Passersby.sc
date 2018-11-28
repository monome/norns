// CroneEngine_Passersby
// West coast style mono synth with complex waveform generation, basic FM and a lowpass gate.
// v1.1.0 Mark Eats

Engine_Passersby : CroneEngine {

	var lfos;
	var oscMods;
	var additiveGroup;
	var synthVoice;
	var reverb;

	var lfosBus;
	var freqBus;
	var modFreqBus;
	var phaseBus;
	var additiveOscBus;
	var fxBus;

	var numLfoDests = 9;
	var lfoDests;
	var noteList;
	var activeNoteId;
	var waveShapeModPoll, waveFoldsModPoll, fm1AmountModPoll, fm2AmountModPoll, attackModPoll, peakMulModPoll, decayModPoll, reverbMixModPoll;

	var startPauseRoutine;
	var pauseRoutine;
	var decay = 1;

	*new { arg context, doneCallback;
		^super.new(context, doneCallback);
	}

	alloc {

		Routine({

			lfosBus = Bus.control(server: context.server, numChannels: numLfoDests);
			freqBus = Bus.control(server: context.server, numChannels: 1);
			modFreqBus = Bus.audio(server: context.server, numChannels: 1);
			phaseBus = Bus.audio(server: context.server, numChannels: 1);
			additiveOscBus = Bus.audio(server: context.server, numChannels: 1);
			fxBus = Bus.audio(server: context.server, numChannels: 1);

			noteList = List.new();

			// LFOs
			SynthDef(\lfos, {

				arg out, lfoShape = 0, lfoFreq = 0.5,
				lfoToFreqAmount = 0, lfoToWaveShapeAmount = 0, lfoToWaveFoldsAmount = 0, lfoToFm1Amount = 0, lfoToFm2Amount = 0,
				lfoToAttackAmount = 0, lfoToPeakAmount = 0, lfoToDecayAmount = 0, lfoToReverbMixAmount = 0, drift = 0;
				var i_driftRate = 0.15, i_outIds = #[0, 1, 2, 3, 4, 5, 6, 7, 8], outArray, lfo;

				// Osc
				lfo = Select.kr(lfoShape, [
					LFTri.kr(lfoFreq),
					LFSaw.kr(lfoFreq),
					LFPulse.kr(lfoFreq),
					LFDNoise0.kr(lfoFreq * 2)
				]);

				// Drift and scale
				outArray = Array.fill(numLfoDests, 0);
				outArray[0] = (lfo * lfoToFreqAmount * 18).midiratio; // Freq ratio
				outArray[1] = (lfo * lfoToWaveShapeAmount) + LFNoise1.kr(freq: i_driftRate, mul: drift); // Wave Shape
				outArray[2] = ((lfo * lfoToWaveFoldsAmount) + LFNoise1.kr(freq: i_driftRate, mul: drift)) * 2; // Wave Folds
				outArray[3] = ((lfo * lfoToFm1Amount) + LFNoise1.kr(freq: i_driftRate, mul: drift)) * 0.5; // FM1 Amount
				outArray[4] = ((lfo * lfoToFm2Amount) + LFNoise1.kr(freq: i_driftRate, mul: drift)) * 0.5; // FM2 Amount
				outArray[5] = ((lfo * lfoToAttackAmount) + LFNoise1.kr(freq: i_driftRate, mul: drift)) * 2.2; // Attack
				outArray[6] = (((lfo * lfoToPeakAmount) + LFNoise1.kr(freq: i_driftRate, mul: drift)) * 24).midiratio; // Peak multiplier
				outArray[7] = ((lfo * lfoToDecayAmount) + LFNoise1.kr(freq: i_driftRate, mul: drift)) * 2.2; // Decay
				outArray[8] = (lfo * lfoToReverbMixAmount) + LFNoise1.kr(freq: i_driftRate, mul: drift); // Reverb Mix

				SendTrig.kr(in: Impulse.kr(15), id: i_outIds, value: outArray);
				Out.kr(out, outArray);

			}).add;


			// Osc mods
			SynthDef(\oscMods, {

				arg freqOut, modFreqOut, phaseOut, lfosIn, freq = 220, pitchBendRatio = 1, glide = 0,
				fm1Ratio = 0.66, fm2Ratio = 3.3, fm1Amount = 0.0, fm2Amount = 0.0;

				var i_nyquist = SampleRate.ir * 0.5, modFreq, mod1, mod2, mod1Index, mod2Index, mod1Freq, mod2Freq, controlLag = 0.005;

				// LFO ins
				freq = (freq * In.kr(lfosIn, numLfoDests)[0]).clip(0, i_nyquist);
				fm1Amount = (fm1Amount + In.kr(lfosIn, numLfoDests)[3]).clip(0, 1);
				fm2Amount = (fm2Amount + In.kr(lfosIn, numLfoDests)[4]).clip(0, 1);

				// Lag inputs
				freq = Lag.kr(freq * pitchBendRatio, 0.007 + glide);
				fm1Ratio = Lag.kr(fm1Ratio, controlLag);
				fm2Ratio = Lag.kr(fm2Ratio, controlLag);
				fm1Amount = Lag.kr(fm1Amount.squared, controlLag);
				fm2Amount = Lag.kr(fm2Amount.squared, controlLag);

				// Modulators
				mod1Index = fm1Amount * 22;
				mod1Freq = freq * fm1Ratio * LFNoise2.kr(freq: 0.1, mul: 0.001, add: 1);
				mod1 = SinOsc.ar(freq: mod1Freq, phase: 0, mul: mod1Index * mod1Freq, add: 0);
				mod2Index = fm2Amount * 12;
				mod2Freq = freq * fm2Ratio * LFNoise2.kr(freq: 0.1, mul: 0.005, add: 1);
				mod2 = SinOsc.ar(freq: mod2Freq, phase: 0, mul: mod2Index * mod2Freq, add: 0);
				modFreq = freq + mod1 + mod2;

				Out.kr(freqOut, freq);
				Out.ar(modFreqOut, modFreq);
				Out.ar(phaseOut, LFSaw.ar(freq: modFreq, iphase: 1, mul: pi, add: pi));
			}).add;


			// Additive osc for square and saw
			SynthDef(\additiveOsc, {
				arg out, freqIn, phaseIn, offset = 0, waveShape;
				var i_nyquist = SampleRate.ir * 0.5, i_numHarmonics = 12, signal;

				waveShape = waveShape.linlin(0.666666, 1, 0, 1);

				signal = Mix.fill(i_numHarmonics, {
					arg index;
					var harmonic, harmonicFreq, harmonicCutoff, attenuation;

					harmonic = index + 1 + (i_numHarmonics * offset);
					harmonicFreq = In.kr(freqIn, 1) * harmonic;
					harmonicCutoff = i_nyquist - harmonicFreq;

					// Attenuate harmonics that will go over nyquist once FM is applied
					attenuation = Select.kr(index, [1, // Save the fundamental
						(harmonicCutoff - (harmonicFreq * 0.25) - harmonicFreq).expexp(0.000001, harmonicFreq * 0.5, 0.000001, 1)]);

					(sin(In.ar(phaseIn, 1) * harmonic % 2pi) / harmonic) * attenuation * (harmonic % 2 + waveShape).min(1);
				});

				Out.ar(out, signal * 0.25);
			}).add;


			// Synth voice
			SynthDef(\synthVoice, {

				arg out, lfosIn, modFreqIn, additiveOscIn, t_gate, gate, killGate, vel = 0.7, pressure = 0, timbre = 0,
				waveShape = 0, waveFolds = 0, envType = 0, attack = 0.04, peak = 10000, decay = 1, amp = 1;

				var i_nyquist = SampleRate.ir * 0.5, signal, controlLag = 0.005,
				sinOsc, triOsc, filterEnvVel, filterEnvLow, lpgEnvelope, lpgSignal, asrEnvelope, asrFilterFreq, asrSignal, killEnvelope;

				// LFO ins
				waveShape = (waveShape + In.kr(lfosIn, numLfoDests)[1]).clip(0, 1);
				waveFolds = (waveFolds + In.kr(lfosIn, numLfoDests)[2]).clip(0, 3);
				attack = (attack + In.kr(lfosIn, numLfoDests)[5]).clip(0.003, 8);
				peak = (peak * In.kr(lfosIn, numLfoDests)[6]).clip(100, 10000);
				decay = (decay + In.kr(lfosIn, numLfoDests)[7]).clip(0.01, 8);

				// Lag inputs
				vel = Lag.kr(vel, controlLag);
				waveShape = Lag.kr(waveShape, controlLag);
				waveFolds = Lag.kr(waveFolds, controlLag);
				attack = Lag.kr(attack, controlLag);
				peak = Lag.kr(peak, controlLag);
				decay = Lag.kr(decay, controlLag);

				// Carrier waves
				sinOsc = SinOsc.ar(freq: In.ar(modFreqIn, 1), phase: 0, mul: 0.5);
				triOsc = VarSaw.ar(freq: In.ar(modFreqIn, 1), iphase: 0, width: 0.5, mul: 0.5);
				signal = LinSelectX.ar(waveShape * 3, [sinOsc, triOsc, In.ar(additiveOscIn, 1)]);

				// Fold
				signal = Fold.ar(in: signal * (1 + (timbre * 0.5) + (waveFolds * 2)), lo: -0.5, hi: 0.5);

				// Hack away some aliasing
				signal = LPF.ar(in: signal, freq: 12000);

				// Noise
				signal = signal + PinkNoise.ar(mul: 0.003);

				// LPG
				filterEnvVel = vel.linexp(0, 1, 0.5, 1);
				filterEnvLow = (peak * filterEnvVel).min(300);

				lpgEnvelope = EnvGen.ar(envelope: Env.new(levels: [0, 1, 0], times: [0.003, decay], curve: [4, -20]), gate: t_gate);
				lpgSignal = RLPF.ar(in: signal, freq: lpgEnvelope.linlin(0, 1, filterEnvLow, peak * filterEnvVel), rq: 0.9);
				lpgSignal = lpgSignal * EnvGen.ar(envelope: Env.new(levels: [0, 1, 0], times: [0.002, decay], curve: [4, -10]), gate: t_gate);

				// ASR with 4-pole filter
				asrEnvelope = EnvGen.ar(envelope: Env.new(levels: [0, 1, 0], times: [attack, decay], curve: -4, releaseNode: 1), gate: gate);
				asrFilterFreq = asrEnvelope.linlin(0, 1, filterEnvLow, peak * filterEnvVel);
				asrSignal = RLPF.ar(in: signal, freq: asrFilterFreq, rq: 0.95);
				asrSignal = RLPF.ar(in: asrSignal, freq: asrFilterFreq, rq: 0.95);
				asrSignal = asrSignal * EnvGen.ar(envelope: Env.asr(attackTime: attack, sustainLevel: 1, releaseTime: decay, curve: -4), gate: gate);

				signal = Select.ar(envType, [lpgSignal, asrSignal]);

				killEnvelope = EnvGen.kr(envelope: Env.asr( 0, 1, 0.01), gate: killGate);
				signal = signal * vel.linlin(0, 1, 0.2, 1) * killEnvelope;

				// Saturation amp
				signal = tanh(signal * pressure.linlin(0, 1, 1.5, 3) * amp).softclip;

				Out.ar(out, signal);

			}).add;


			// Very approx spring reverb
			SynthDef(\reverb, {

				arg in, out, lfosIn, mix = 0;
				var dry, preProcess, springReso, wet, predelay = 0.015;

				mix = (mix + In.kr(lfosIn, numLfoDests)[8]).clip(0, 1);
				mix = Lag.kr(mix, 0.01);

				dry = In.ar(in, 1);

				preProcess = tanh(BHiShelf.ar(in: dry, freq: 1000, rs: 1, db: -6, mul: 1.5, add: 0)); // Darken and saturate
				preProcess = DelayN.ar(in: preProcess, maxdelaytime: predelay, delaytime: predelay);
				springReso = Klank.ar(specificationsArrayRef: `[[508, 270, 1153], [0.15, 0.25, 0.1], [1, 1.2, 1.4]], input: preProcess);
				springReso = Limiter.ar(springReso).dup;
				preProcess = preProcess * 0.55; // FreeVerb doesn't like a loud signal
				wet = tanh(FreeVerb2.ar(in: preProcess, in2: preProcess, mix: 1, room: 0.7, damp: 0.35, mul: 1.8));
				wet = (wet * 0.935) + (springReso * 0.065);

				Out.ar(out, (dry.dup * (1 - mix)) + (wet * mix));

			}).add;

			context.server.sync; // Wait for all the SynthDefs to be added on the server

			lfos = Synth(defName: \lfos, args: [\out, lfosBus], target: context.xg);

			oscMods = Synth.newPaused(defName: \oscMods, args: [
				\freqOut, freqBus,
				\modFreqOut, modFreqBus,
				\phaseOut, phaseBus,
				\lfosIn, lfosBus
			], target: context.xg, addAction: \addToTail);

			additiveGroup = ParGroup.new(context.xg, \addToTail);
			4.do({
				arg index;
				Synth.newPaused(defName: \additiveOsc, args: [\out, additiveOscBus, \freqIn, freqBus, \phaseIn, phaseBus, \offset, index], target: additiveGroup);
			});

			synthVoice = Synth.newPaused(defName: \synthVoice, args: [
				\out, fxBus,
				\lfosIn, lfosBus,
				\modFreqIn, modFreqBus,
				\additiveOscIn, additiveOscBus
			], target: context.xg, addAction: \addToTail);

			reverb = Synth(defName: \reverb, args: [
				\in, fxBus,
				\out, context.out_b,
				\lfosIn, lfosBus
			], target: context.xg, addAction: \addToTail);

		}).play;


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
				5, { attackModPoll.update(value) },
				6, { peakMulModPoll.update(value) },
				7, { decayModPoll.update(value) },
				8, { reverbMixModPoll.update(value) }
			);
		}, path: '/tr', srcID: context.server.addr);

		// Polls
		waveShapeModPoll = this.addPoll(name: "waveShapeMod", periodic: false);
		waveFoldsModPoll = this.addPoll(name: "waveFoldsMod", periodic: false);
		fm1AmountModPoll = this.addPoll(name: "fm1AmountMod", periodic: false);
		fm2AmountModPoll = this.addPoll(name: "fm2AmountMod", periodic: false);
		attackModPoll = this.addPoll(name: "attackMod", periodic: false);
		peakMulModPoll = this.addPoll(name: "peakMulMod", periodic: false);
		decayModPoll = this.addPoll(name: "decayMod", periodic: false);
		reverbMixModPoll = this.addPoll(name: "reverbMixMod", periodic: false);


		// Commands

		// noteOn(id, freq, vel)
		this.addCommand(\noteOn, "iff", { arg msg;

			var id = msg[1], freq = msg[2], vel = msg[3], note = Dictionary.new(2);

			noteList.remove(noteList.detect{arg item; item[\id] == id});

			if(oscMods.notNil && additiveGroup.notNil && synthVoice.notNil, {
				pauseRoutine.stop;
				oscMods.run(true);
				additiveGroup.run(true);
				synthVoice.run(true);

				oscMods.set(\freq, freq);
				synthVoice.set(\vel, vel, \t_gate, 1, \gate, 1, \killGate, 1);
				note[\id] = id;
				note[\freq] = freq;
				noteList.add(note);
				activeNoteId = id;
			});
		});

		startPauseRoutine = {
			pauseRoutine = Routine {
				(decay + 0.01).wait;
				if(oscMods.notNil && additiveGroup.notNil && synthVoice.notNil, {
					oscMods.run(false);
					additiveGroup.run(false);
					synthVoice.run(false);
				});
			}.play;
		};

		// noteOff(id)
		this.addCommand(\noteOff, "i", { arg msg;

			var id = msg[1];

			noteList.remove(noteList.detect{arg item; item[\id] == id});

			if(id == activeNoteId, {
				if(noteList.size > 0, {
					oscMods.set(\freq, noteList.last.[\freq]);
					activeNoteId = noteList.last.[\id];
				}, {
					synthVoice.set(\gate, 0);
					activeNoteId = Nil;
					pauseRoutine.stop;
					startPauseRoutine.value;
				});
			});
		});

		// noteOffAll()
		this.addCommand(\noteOffAll, "", { arg msg;
			synthVoice.set(\gate, 0);
			activeNoteId = Nil;
			noteList.clear;
			pauseRoutine.stop;
			startPauseRoutine.value;
		});

		// noteKill(id)
		this.addCommand(\noteKill, "i", { arg msg;

			var id = msg[1];

			noteList.remove(noteList.detect{arg item; item[\id] == id});

			if(id == activeNoteId, {
				if(noteList.size > 0, {
					oscMods.set(\freq, noteList.last.[\freq]);
					activeNoteId = noteList.last.[\id];
				}, {
					synthVoice.set(\gate, 0);
					synthVoice.set(\killGate, 0);
					activeNoteId = Nil;
					pauseRoutine.stop;
					startPauseRoutine.value;
				});
			});
		});

		// noteKillAll()
		this.addCommand(\noteKillAll, "", { arg msg;
			synthVoice.set(\gate, 0);
			synthVoice.set(\killGate, 0);
			activeNoteId = Nil;
			noteList.clear;
			pauseRoutine.stop;
			startPauseRoutine.value;
		});

		// pitchBend(id, ratio)
		this.addCommand(\pitchBend, "if", { arg msg;
			oscMods.set(\pitchBendRatio, msg[2]);
		});

		// pitchBendAll(ratio)
		this.addCommand(\pitchBendAll, "f", { arg msg;
			oscMods.set(\pitchBendRatio, msg[1]);
		});

		// pressure(id, pressure)
		this.addCommand(\pressure, "if", { arg msg;
			synthVoice.set(\pressure, msg[2]);
		});

		// pressureAll(pressure)
		this.addCommand(\pressureAll, "f", { arg msg;
			synthVoice.set(\pressure, msg[1]);
		});

		// timbre(id, timbre)
		this.addCommand(\timbre, "if", { arg msg;
			synthVoice.set(\timbre, msg[2]);
		});

		// timbreAll(timbre)
		this.addCommand(\timbreAll, "f", { arg msg;
			synthVoice.set(\timbre, msg[1]);
		});

		this.addCommand(\glide, "f", { arg msg;
			oscMods.set(\glide, msg[1]);
		});

		this.addCommand("waveShape", "f", { arg msg;
			var waveShape = msg[1];
			additiveGroup.set(\waveShape, waveShape);
			synthVoice.set(\waveShape, waveShape);
		});

		this.addCommand("waveFolds", "f", { arg msg;
			synthVoice.set(\waveFolds, msg[1]);
		});

		this.addCommand("fm1Ratio", "f", { arg msg;
			oscMods.set(\fm1Ratio, msg[1]);
		});

		this.addCommand("fm2Ratio", "f", { arg msg;
			oscMods.set(\fm2Ratio, msg[1]);
		});

		this.addCommand("fm1Amount", "f", { arg msg;
			oscMods.set(\fm1Amount, msg[1]);
		});

		this.addCommand("fm2Amount", "f", { arg msg;
			oscMods.set(\fm2Amount, msg[1]);
		});

		this.addCommand("envType", "i", { arg msg;
			synthVoice.set(\envType, msg[1]);
		});

		this.addCommand("attack", "f", { arg msg;
			synthVoice.set(\attack, msg[1]);
		});

		this.addCommand("peak", "f", { arg msg;
			synthVoice.set(\peak, msg[1]);
		});

		this.addCommand("decay", "f", { arg msg;
			decay = msg[1];
			synthVoice.set(\decay, decay);
		});

		this.addCommand("amp", "f", { arg msg;
			synthVoice.set(\amp, msg[1]);
		});

		this.addCommand("reverbMix", "f", { arg msg;
			reverb.set(\mix, msg[1]);
		});

		this.addCommand("lfoShape", "i", { arg msg;
			lfos.set(\lfoShape, msg[1]);
		});

		this.addCommand("lfoFreq", "f", { arg msg;
			lfos.set(\lfoFreq, msg[1]);
		});

		this.addCommand("lfoToFreqAmount", "f", { arg msg;
			lfos.set(\lfoToFreqAmount, msg[1]);
		});

		this.addCommand("lfoToWaveShapeAmount", "f", { arg msg;
			lfos.set(\lfoToWaveShapeAmount, msg[1]);
		});

		this.addCommand("lfoToWaveFoldsAmount", "f", { arg msg;
			lfos.set(\lfoToWaveFoldsAmount, msg[1]);
		});

		this.addCommand("lfoToFm1Amount", "f", { arg msg;
			lfos.set(\lfoToFm1Amount, msg[1]);
		});

		this.addCommand("lfoToFm2Amount", "f", { arg msg;
			lfos.set(\lfoToFm2Amount, msg[1]);
		});

		this.addCommand("lfoToAttackAmount", "f", { arg msg;
			lfos.set(\lfoToAttackAmount, msg[1]);
		});

		this.addCommand("lfoToPeakAmount", "f", { arg msg;
			lfos.set(\lfoToPeakAmount, msg[1]);
		});

		this.addCommand("lfoToDecayAmount", "f", { arg msg;
			lfos.set(\lfoToDecayAmount, msg[1]);
		});

		this.addCommand("lfoToReverbMixAmount", "f", { arg msg;
			lfos.set(\lfoToReverbMixAmount, msg[1]);
		});

		this.addCommand("drift", "f", { arg msg;
			lfos.set(\drift, msg[1]);
		});

	}

	free {
		lfos.free;
		oscMods.free;
		additiveGroup.free;
		synthVoice.free;
		reverb.free;
	}
}
