Engine_Knaster : CroneEngine {
	classvar volumeIsParameter = false; // TODO
	var <group;
	var volumeBus; // TODO
	var synth;

	*knasterSynthDef {
		^SynthDef.new('knaster', {
			arg out, volume;

            var density = 4;
			var sig;
			var panmod, delaymod;
			var delaySpec;

			sig = Dust2.ar(density);

			panmod = LFNoise1.ar(1.8);
			sig = Pan2.ar(sig, panmod);

			sig = FreeVerb2.ar(sig[0], sig[1], room:0.2, damp: 0);

			delaySpec = \delay.asSpec.copy.maxval_(4);
			delaymod = LFNoise1.ar(2.2).range(0.01, 1);
			sig = sig + CombC.ar(sig, maxdelaytime: delaySpec.maxval, delaytime: delaySpec.map(delaymod), decaytime: 1);

			sig = sig * \db.asSpec.unmap(volume);

			Out.ar(out, sig);
		}, metadata: (specs: (volume: \db.asSpec)));
	}

	*initClass {
		Class.initClassTree(ControlSpec);
	}

	*new { |context, callback| ^super.new(context, callback) }

	alloc {
		this.class.knasterSynthDef.add;

		context.server.sync;

		group = Group.new(context.xg);

		volumeBus = if (volumeIsParameter) {
			// TODO: parameterControlBusses[\volume];
		} { Bus.control };

		context.server.bind { // TODO
		    volumeBus.set(0); // TODO
			synth = Synth.new(
				'knaster',
				[
					\out, context.out_b,
					\volume, volumeBus.asMap,
				],
				group
			);
		};

		this.addCommand(\run, "i", { arg msg;
			synth.run(msg[1]);
		});

		if (volumeIsParameter) {
			this.addParameter(\volume, this.class.knasterSynthDef.metadata.specs[\volume]); // TODO: parameters
		} {
			this.addCommand(\volume, "f", { arg msg;
				volumeBus.set(msg[1]);
			});
		};

		this.addCommand(\density, "i", { arg msg;
			synth.set(\density, msg[1]);
		});
	}

	free {
		group.free;
		if (volumeIsParameter.not) {
			volumeBus.free;
		};
		super.free;
	}
}
