Engine_Shift : CroneGenEngine {
	*ugenGraphFunc {
		^{
			arg
				in,
				out,
				pitch_ratio,
				pitch_dispersion,
				time_dispersion,
				freqshift_freq
/*
	TODO
				amp_env0,
				amp_env1,
				pitch_trk0,
				pitch_trk1,
				ampenv_to_osc1freq,
				ampenv_to_osc2freq,
				ampenv_to_osc3freq,
				pitchenv_to_osc1freq,
				pitchenv_to_osc2freq,
				pitchenv_to_osc3freq
*/
			;

			var sig;
			sig = In.ar(in, 2);
			sig = PitchShift.ar(sig, 0.2, pitch_ratio, pitch_dispersion, time_dispersion);
			sig = FreqShift.ar(sig, freqshift_freq);
			Out.ar(out, sig); // TODO: stereo output?
		}
	}

    *specs {
		^(
			pitch_ratio: ControlSpec(0, 4, default: 1),
			pitch_dispersion: ControlSpec(0, 4),
			time_dispersion: ControlSpec(0, 1),
			freqshift_freq: ControlSpec(-2000, 2000, default: 0, units: "Hz"),
		)
    }

	*synthDef { // TODO: remove, this is just due to wrapping of out not working right atm
		^SynthDef(
			\abcd,
			this.ugenGraphFunc,
			metadata: (specs: this.specs)
		)
	}
}

