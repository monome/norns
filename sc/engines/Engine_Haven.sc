/*
2019, Till Bovermann
http://tai-studio.org


*/

Engine_Haven : CroneGenEngine {
	*specs {
		^(
			\freq1: [1, 800, \exp, 0.0, 20].asSpec,
			\freq2: [400, 12000, \exp, 0.0, 4000, "Hz"].asSpec,
			\amp1: [-90, 0, \linear, 0.0, 0, ""].asSpec,
			\amp2: [-90, 0, \linear, 0.0, 0, ""].asSpec,
			\inAmp: [-90, 0, \linear, 0.0, 0, ""].asSpec,
			\fdbck: [0, 1, \linear, 0.0, 0.03, ""].asSpec,
			\fdbckSign: [-1, 1, \linear, 1, 0, ""].asSpec
		)
	}

	*synthDef { // TODO: move ugenGraphFunc to here...
		^SynthDef(\haven, {|in = 0, out = 0, freq1 = 20, freq2 = 4000, amp1 = -90, amp2 = -90, inAmp = -90, fdbck = 0.3, fdbckSign = 1|
			var freqs, freqRanges;
			var inputs, oscAmps;
			var dyns, dynIns;
			var xxx, snd, lIns;
			var rotate = {|in, pos = 0.0|
				Rotate2.ar(in[0], in[1], pos)
			};

			oscAmps = [amp1, amp2];
			oscAmps = oscAmps.varlag(0.3);
			inAmp = inAmp.varlag(0.3);

			oscAmps = max(0, oscAmps.dbamp - (-90.dbamp)); // ensure mute when at -90 db;
			inAmp   = max(0,  inAmp .dbamp - (-90.dbamp)); // ensure mute when at -90 db;




			// combine fdbck with its sign
			fdbck = fdbck.varlag(0.3) * fdbckSign.lag(2.0 / SampleRate.ir);

			lIns = LocalIn.ar(2);
			inputs = (In.ar(in, 2) * inAmp);
			dynIns = Amplitude.ar(inputs, 0.1, 0.1);

			// magic (>:)
			xxx = Limiter.ar(
				CombL.ar(
					lIns/* + inputs*/,
					0.5,
					(LFNoise1.ar(0.1, 0.5, 0.5) + dynIns).tanh * 0.5,
					-10
				)
			);

			dyns = Impulse.ar(0).lag(0.001, 0.1)
			+ Amplitude.ar(
				in: xxx.reverse,
				attackTime: LFNoise1.kr(dynIns[0].lag(1) * 0.1).abs * 5,
				releaseTime: LFNoise1.kr(dynIns[0].lag(3) * 0.5).abs * 15
			);

			freqRanges = [0.85, 0.4];
			freqs = [freq1, freq2].varlag(0.5);
			freqs = freqs -
			(
				dyns.lag(0.003235246)
				* (
					(freqRanges * freqs)
				)
			);


			// oscAmps = [amp1, amp2].dbamp.poll;
			oscAmps = oscAmps * AmpCompA.kr(freqs);
			snd = [
				SinOscFB.ar(
					freq: freqs[0],
					feedback: (dyns[0] * 2).fold(0, 1.5)
				) * oscAmps[0] * 10,
				RHPF.ar(
					Pulse.ar(
						freq: freqs[1],
						width: (dyns[1].lag(0.01) * 200.1 + Decay.kr(Dust.kr(50 * (1-dyns[0])), 0.05)).wrap(0, 1),
					) * oscAmps[1] * 4,
					8 * freqs[1].varlag(10, 20, start: 400), 0.3
				)
			];



			//stereo rotate
			snd = rotate.(
				in: snd,
				pos: (dynIns + LFSaw.kr(0.001, 0.23))%1 // (2, 2)
			);
			// mix with input
			snd = (inputs + snd).tanh;
			// ensure silence on load
			snd = snd * EnvGen.kr(Env([0, 0, 1], [1.2, 0.5]));

			// amp modulation
			snd = snd * SinOsc.ar(
				(0.01 + dyns.varlag(2, 1, 5, 1)) * 0.5,
				{Rand()}!2
			).range((dyns.varlag(2, 1, 5, 10) * 0.5) + 0.5, 1);/* * SinOsc.ar(
			(1-dyns.lag([1, 1.2])) * 20,
			{Rand()}!2
			).range(0.01, 1);
			*/


			snd = (fdbck * xxx) // feedback
			+ snd;

			// collapse to stereo
			snd = snd.sum;
			snd = LeakDC.ar(snd);

			LocalOut.ar(snd);

			snd = (snd * 0.8) - (0.5 * MoogLadder.ar(rotate.(snd, dyns.sum.lag(0.01)), ffreq: (1-dyns.lag(0, 10)) * 1100 + 50, res: 0.2));
			snd = snd.tanh;

			Out.ar(out, snd);
		},
		metadata: (specs: this.specs)
		)
	}
}

/*

Engine_Haven.generateLuaEngineModuleSpecsSection
*/