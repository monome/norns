Engine_Knaster : CroneEngine {
	classvar defName = \knaster;
	classvar <specs;
	var <group;
	var synth;

	*initClass {
		Class.initClassTree(ControlSpec);
		specs = IdentityDictionary[
			\db -> \db.asSpec
		];
		StartUp.add {
			var synthDef = SynthDef.new(defName, {
				arg out, gate=1, meh, db;
				var sig;
				var panmod, delaymod, delaytime;
				var hpfCutoff = meh;
				var freq = meh;
				var delaySpec = \delay.asSpec.copy.maxval_(4);
				var rhpfFreqSpec = ControlSpec(1, 6000, 'exp', 0, 440, " Hz");
				var dustFreqSpec = ControlSpec(1.5, 100, 'exp', 0, 6, " Hz");

				panmod = LFNoise1.ar(1.8);
				delaymod = LFNoise1.ar(2.2).range(0.01, 1);
				sig = Dust2.ar(dustFreqSpec.map(freq));
				sig = HPF.ar(sig, rhpfFreqSpec.map(hpfCutoff));
				sig = Pan2.ar(sig, panmod);
				sig = FreeVerb2.ar(sig[0], sig[1], room:1, damp: 0.3);

				delaytime = delaySpec.map(delaymod);
				sig = sig + CombC.ar(sig, maxdelaytime: delaySpec.maxval, delaytime: delaytime, decaytime: 1);
				sig = sig * \db.asSpec.unmap(In.kr(db));
				sig = sig * Env.cutoff(0.01).ar(2, gate);
				Out.ar(out, sig);
			});

			CroneDefs.add(synthDef);
		}
	}

	*new { arg context; ^super.new.init(context).initEngine_Knaster; }

	initEngine_Knaster {
		context.postln;
		group = Group.new(context.xg);

		this.addCommand(\start, "", { arg msg;
			if (synth == nil) {
				synth = Synth.new(
					defName,
					[
						\out, context.out_b.index,
						\db, parameterControlBusses[\db],
						\meh, 0.333
					],
					group
				);
			};
		});

		this.addCommand(\stop, "", { arg msg;
			if (synth.notNil) {
				synth.release;
				synth = nil;
			}
		});

		this.addParameter(\db, specs[\db]);
	}

	free {
		group.free;
		super.free;
	}
}
