Engine_Bob : CroneGenEngine {
	*ugenGraphFunc {
		^{
			arg
				in,
				out,
				cutoff,
				resonance
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

			Out.ar(out, MoogLadder.ar(In.ar(in, 2), cutoff, resonance));
		}
	}

    *specs {
		^(
			cutoff: \freq.asSpec,
			resonance: \unipolar.asSpec
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

