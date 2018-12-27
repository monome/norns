Engine_Gong : CroneGenEngine {
	classvar numOscs = 3;

	// TODO: not used atm *polyphony { ^6 }

	*ugenGraphFunc {
		^{
			arg
				out,
				gate,
				freq,
				timbre,
				timemod,
				osc1gain,
				osc1partial,
				// TODO osc1partialdetune,
				osc1fixed,
				osc1fixedfreq,
				osc1index,
				osc1outlevel,
				osc1_to_osc1freq,
				osc1_to_osc2freq,
				osc1_to_osc3freq,
				osc2gain,
				osc2partial,
				osc2fixed,
				osc2fixedfreq,
				osc2index,
				osc2outlevel,
				osc2_to_osc1freq,
				osc2_to_osc2freq,
				osc2_to_osc3freq,
				osc3gain,
				osc3partial,
				osc3fixed,
				osc3fixedfreq,
				osc3index,
				osc3outlevel,
				osc3_to_osc3freq,
				osc3_to_osc2freq,
				osc3_to_osc1freq,
				filtermode,
				filtercutoff,
				filterres,
				ampgain,
				lforate,
				lfo_to_ampgain,
				lfo_to_filtercutoff,
				lfo_to_filterres,
				lfo_to_osc1freq,
				lfo_to_osc2freq,
				lfo_to_osc3freq,
				lfo_to_osc1gain,
				lfo_to_osc2gain,
				lfo_to_osc3gain,
				envattack,
				envdecay,
				envsustain,
				envrelease,
				envcurve,
				env_to_osc1freq,
				env_to_osc1gain,
				env_to_osc2freq,
				env_to_osc2gain,
				env_to_osc3freq,
				env_to_osc3gain,
				env_to_filtercutoff,
				env_to_filterres,
				env_to_hpfcutoff,
				env_to_hpfres,
				env_to_ampgain
/*
	TODO
				pitch_trk0,
				pitch_trk1,
				pitchtrk0_to_osc1freq,
				pitchtrk0_to_osc2freq,
				pitchtrk0_to_osc3freq,
				pitchtrk1_to_osc1freq,
				pitchtrk1_to_osc2freq,
				pitchtrk1_to_osc3freq,
				amp_env0,
				amp_env1
				ampenv_to_osc1freq,
				ampenv_to_osc2freq,
				ampenv_to_osc3freq,
*/
			;
			var sig;
			var env = EnvGen.ar(Env.adsr((envattack*timemod).clip(0, 5000)/1000, (envdecay*timemod).clip(0, 5000)/1000, envsustain, (envrelease*timemod).clip(0, 5000)/1000, curve: envcurve), gate);
			var freqSpec = ControlSpec(20, 10000, 'exp', 0, 440, " Hz");
			var rqSpec = \rq.asSpec;
			var lfo = SinOsc.ar((lforate/timemod).clip(0.125, 8));

			var osc1, osc2, osc3;
			var osc1freq, osc2freq, osc3freq;
			var osc1freqbasemod, osc2freqbasemod, osc3freqbasemod;
			var oscfeedback = LocalIn.ar(3);

			osc1freq = Select.kr(osc1fixed, [freq*osc1partial, osc1fixedfreq]);
			osc2freq = Select.kr(osc2fixed, [freq*osc2partial, osc2fixedfreq]);
			osc3freq = Select.kr(osc3fixed, [freq*osc3partial, osc3fixedfreq]);

			osc1freqbasemod = osc1index * osc1freq * timbre;
			osc2freqbasemod = osc2index * osc2freq * timbre;
			osc3freqbasemod = osc3index * osc3freq * timbre;

			osc1 = SinOsc.ar(
				osc1freq
					+ (osc1freqbasemod * oscfeedback[0] * osc1_to_osc1freq) // TODO: moving index multiplication is likely more optimal
					+ (osc1freqbasemod * oscfeedback[1] * osc2_to_osc1freq)
					+ (osc1freqbasemod * oscfeedback[2] * osc3_to_osc1freq)
					+ (osc1freqbasemod * env * env_to_osc1freq)
					+ (osc1freqbasemod * lfo * lfo_to_osc1freq)
			) * (osc1gain + (env_to_osc1gain * env) + (lfo * lfo_to_osc1gain)).clip(0, 1);

			osc2 = SinOsc.ar(
				osc2freq
					+ (osc2freqbasemod * osc1 * osc1_to_osc2freq) // TODO: moving index multiplication is likely more optimal
					+ (osc2freqbasemod * oscfeedback[1] * osc2_to_osc2freq)
					+ (osc2freqbasemod * oscfeedback[2] * osc3_to_osc2freq)
					+ (osc2freqbasemod * env * env_to_osc2freq)
					+ (osc2freqbasemod * lfo * lfo_to_osc2freq)
			) * (osc2gain + (env_to_osc2gain * env) + (lfo * lfo_to_osc2gain)).clip(0, 1);

			osc3 = SinOsc.ar(
				osc3freq
					+ (osc3freqbasemod * osc1 * osc1_to_osc3freq) // TODO: moving index multiplication is likely more optimal
					+ (osc3freqbasemod * osc2 * osc2_to_osc3freq)
					+ (osc3freqbasemod * oscfeedback[2] * osc3_to_osc3freq)
					+ (osc3freqbasemod * env * env_to_osc3freq)
					+ (osc3freqbasemod * lfo * lfo_to_osc3freq)
			) * (osc3gain + (env_to_osc3gain * env) + (lfo * lfo_to_osc3gain)).clip(0, 1);

			sig = (osc1 * osc1outlevel) + (osc2 * osc2outlevel) + (osc3 * osc3outlevel);

			sig = SVF.ar(
				signal: sig,
				cutoff: freqSpec.map(freqSpec.unmap(filtercutoff)+(env * env_to_filtercutoff) + (lfo * lfo_to_filtercutoff)),
				res: (filterres+(env * env_to_filterres) + (lfo * lfo_to_filterres)).clip(0, 1),
				lowpass: filtermode < 1,
				bandpass: (filtermode > 0) * (filtermode < 2),
				highpass: (filtermode > 1) * (filtermode < 3),
				notch: (filtermode > 2) * (filtermode < 4),
				peak: (filtermode > 3) * (filtermode < 5)
			);

			sig = sig * (ampgain + (env * env_to_ampgain) + (lfo * lfo_to_ampgain)).clip(0, 1);
			LocalOut.ar([osc1, osc2, osc3]);
			Out.ar(out, sig ! 2); // TODO: stereo output?
		}
	}

    *specs {
		var sp;

		sp = sp.add("timbre" -> ControlSpec(0, 5, 'lin', nil, 1, ""));
		sp = sp.add("timemod" -> ControlSpec(1, 5, 'lin', nil, 1, ""));

		numOscs.do { |oscnum|
			sp = sp.addAll(
				[
					"osc%gain".format(oscnum+1) -> \amp.asSpec,
					"osc%partial".format(oscnum+1) -> ControlSpec(0.5, 12, 'lin', 0.5, 1, ""),
					"osc%fixed".format(oscnum+1) -> ControlSpec(0, 1, 'lin', 1, 0, ""),
					"osc%fixedfreq".format(oscnum+1) -> \widefreq.asSpec,
					"osc%index".format(oscnum+1) -> ControlSpec(0, 24, 'lin', 0, 3, ""),
					"osc%outlevel".format(oscnum+1) -> \amp.asSpec,
					"env_to_osc%freq".format(oscnum+1) -> \bipolar.asSpec,
					"env_to_osc%gain".format(oscnum+1) -> \bipolar.asSpec,
					"lfo_to_osc%freq".format(oscnum+1) -> \bipolar.asSpec,
					"lfo_to_osc%gain".format(oscnum+1) -> \bipolar.asSpec,
				]
			);
			numOscs.do { |dest|
				sp = sp.add(
					"osc%_to_osc%freq".format(oscnum+1, dest+1) -> \amp.asSpec
				);
			};
		};
		sp = sp.addAll(
			[
				'filtermode' -> ControlSpec(0, 5, 'lin', 1, 0, ""),
				'filtercutoff' -> ControlSpec(20, 10000, 'exp', 0, 10000, "Hz"),
				'filterres' -> \unipolar.asSpec,
				'ampgain' -> \amp.asSpec,
				'lforate' -> ControlSpec(0.125, 8, 'exp', 0, 1, "Hz"), // TODO \rate.asSpec,
				'lfo_to_filtercutoff' -> \bipolar.asSpec,
				'lfo_to_filterres' -> \bipolar.asSpec,
				'lfo_to_ampgain' -> \bipolar.asSpec,
				'gate' -> \unipolar.asSpec,
				'envattack' -> ControlSpec(0, 5000, 'lin', 0, 5, "ms"),
				'envdecay' -> ControlSpec(0, 5000, 'lin', 0, 400, "ms"),
				'envsustain' -> ControlSpec(0, 1, 'lin', 0, 0.5, ""),
				'envrelease' -> ControlSpec(0, 5000, 'lin', 0, 400, "ms"),
				'envcurve' -> ControlSpec(-20, 20, 'lin', 0, -4, ""),
				'env_to_filtercutoff' -> \bipolar.asSpec,
				'env_to_filterres' -> \bipolar.asSpec,
				'env_to_ampgain' -> \bipolar.asSpec,
				'pitchenv0_to_osc1freq' -> \unipolar.asSpec,
				'pitchenv0_to_osc2freq' -> \unipolar.asSpec,
				'pitchenv0_to_osc3freq' -> \unipolar.asSpec,
				'pitchenv1_to_osc1freq' -> \unipolar.asSpec,
				'pitchenv1_to_osc2freq' -> \unipolar.asSpec,
				'pitchenv1_to_osc3freq' -> \unipolar.asSpec
			]
		);

		sp = sp.collect { |assoc|
			assoc.key.asSymbol -> assoc.value
		};
		// sp.collect{ |assoc| assoc.key}.debug(\debug);

		sp = sp.asDict;

		^sp;
    }

	*synthDef { // TODO: remove, this is just due to wrapping of out not working right atm
		^SynthDef(
			\abcd,
			this.ugenGraphFunc,
			metadata: (specs: this.specs)
		)
	}
}

