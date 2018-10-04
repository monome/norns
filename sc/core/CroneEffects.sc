// this is implemented as a "static" class, and
// kind of quick and dirty in that regard :/

CroneEffects {
	classvar <aux_in_b;
	classvar <in_aux_s;   // patch input to aux
	classvar <out_aux_s;  // patch output to aux
	classvar <aux_gr;    // aux fx group
	classvar <ins_gr;    // insert fx gruop
	classvar <aux_s;     // aux fx synth
	classvar <ins_s;     // insert fx synth

	*init {
		Routine {
			var s, c;
			s = Crone.server;
			c = Crone.context;

			SynthDef.new(\faust_comp, {
				arg in1, in2, out,
				bypass= 0, ratio= 4, threshold= -30, attack= 5, release= 50, makeup_gain= 18;
				var input, snd;
				input = [In.ar(in1), In.ar(in2)];
				snd = FaustCompressor.ar(input[0],	input[1],
					0, ratio, threshold, attack, release, makeup_gain);
				ReplaceOut.ar(out, Mix.new([bypass*input, (1.0-bypass)*snd]));
			}).send(s);


			SynthDef.new(\faust_verb, {
				arg in1, in2, out,
				in_delay= 20.0, lf_x= 200.0, low_rt60= 3.0, mid_rt60= 2.0, hf_damping= 6000.0,
				eq1_freq= 315.0, eq1_level= 0.0, eq2_freq= 1500.0, eq2_level= 0.0, level= 0.25;

				Out.ar(out, FaustZitaRev.ar(In.ar(in1), In.ar(in2), in_delay, lf_x, low_rt60, mid_rt60, hf_damping, eq1_freq, eq1_level, eq2_freq, eq2_level, -1, level));

//				Out.ar(out, FaustZitaVerbLight.ar(In.ar(in1), In.ar(in2), in_delay, lf_x, low_rt60, mid_rt60, hf_damping) * level);
			}).send(s);

			s.sync;

			aux_gr = Group.before(c.og);
			ins_gr = Group.after(aux_gr);

			aux_in_b = Bus.audio(s, 2);

			in_aux_s = Array.fill(2, { |i| Synth.new(\patch_pan,
				[\in, c.in_b[i].index, \out, aux_in_b.index],
				aux_gr, \addBefore); });

			out_aux_s = Synth.new(\patch_stereo,
				[\in, c.out_b.index, \out, aux_in_b.index],
				aux_gr, \addBefore );

			ins_s = Synth.newPaused(\faust_comp, [
				\out, c.out_b.index,
				\in1, c.out_b.index,
				\in2, c.out_b.index + 1
			], target:aux_gr);

			aux_s = Synth.newPaused(\faust_verb, [
				\out, c.out_b.index,
				\in1, aux_in_b.index,
				\in2, aux_in_b.index + 1
			], target:ins_gr, addAction:\addToTail);

			s.sync;

		}.play;
	}


	// input -> aux send level and pan
	*set_in_aux_db { arg chan, val; in_aux_s[chan].set(\level, val.dbamp); }
	*set_in_aux_pan { arg chan, val; in_aux_s[chan].set(\pan, val); }

	// output -> aux send level (stereo)
	*set_out_aux_db { arg val; out_aux_s.set(\level, val.dbamp); }

	// aux return level
	*set_aux_return_db { arg val; aux_s.set(\level, val.dbamp); }

	// set aux synth parameter
	*set_aux_param { arg name, val; aux_s.set(name.asSymbol, val); }

	// enable / disable aux processing
	*aux_enable  { aux_s.run(true); }
	*aux_disable { aux_s.run(false); }

	// set insert mix
	*set_ins_wet_mix { arg val; ins_s.set(\bypass, (1.0 - val).min(1.0).max(0.0)); }

	// enable / disable insert processing
	*ins_enable  { ins_s.run(true); }
	*ins_disable { ins_s.run(false); }

	// set insert synth parameter
	*set_ins_param { arg name, val; ins_s.set(name.asSymbol, val); }


}
