// SuperCollider example from the Rethinking the Computer Music Language article written by James McCartney
Engine_Why : CroneEngine {
	var <synth;

	*new { arg context, doneCallback;
		^super.new(context, doneCallback);
	}

	alloc {
		synth = { |outbus|
			var exciterFunction, numberOfExciters, numberOfCombs, numberOfAllpass;
			var in, predelayed, out;
			// changing these parameters changes the dimensions of the patch
			numberOfExciters = 10;
			numberOfCombs = 7;
			numberOfAllpass = 4;
			// a function to generate the input to the reverb,
			// a pinging sound.
			exciterFunction = { Resonz.ar(Dust.ar(0.2, 50), 200 + rand(3000.0), 0.003) };
			// make a mix of exciters
			// Mix.arFill fills an array with the results of
			// a function and mixes their output
			in = Mix.arFill(numberOfExciters, exciterFunction);
			// reverb predelay time:
			predelayed = DelayN.ar(in, 0.048);
			// a mix of several modulated comb delays in parallel:
			out = Mix.arFill(numberOfCombs, {
				CombL.ar(predelayed, 0.1, LFNoise1.kr(rand(0.1), 0.04, 0.05), 15);
			});
			// a parallel stereo chain of allpass delays.
			// in each iteration of the do loop the Allpass input is the
			// result of the previous iteration
			numberOfAllpass.do({
				out = AllpassN.ar(out, 0.050, [rand(0.050), rand(0.050)], 1);
			});
			// add original sound to reverb and play it:
			Out.ar(outbus, in + (0.2 * out));
		}.play(context.xg, args: [\outbus, context.out_b]);
	}

	free {
		synth.free;
	}
}
