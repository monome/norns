Engine_Vocer : CroneEngine {
	classvar <nvoices = 8;
	
	var <pm;
	var <mixBus;
	var <outPatch;
	var <voices;
	var <sines; // TODO
	var <delay; // TODO
	
	*new { arg context, doneCallback;
		^super.new(context, doneCallback);
	}

	alloc {
		var s = context.server;

		mixBus = Bus.audio(s, 2);
		outPatch = Synth.new(\patch_stereo, [\in, mixBus.index, \out, context.out_b.index],
			target:context.og, addAction:\addBefore);

		voices = Array.fill(nvoices, { arg i;
			VocerVoice.new(s, context.out_b, context.xg);
		});

		pm = Event.new;
		pm.adc_voc = PatchMatrix.new(
			server:s, target:context.ig, action:\addAfter,
			in: Array.fill(2, {|i| context.in_b[i].index}),
			out: voices.collect({|v| v.in_bus.index}),
			feedback:false
		);
		
		#[\gate, \freeze, \shift, \scale, \pan, \atk, \rel].do({
			arg name;
			this.addCommand(name, "if", { 
				arg msg;
				var i = msg[1] -1;
				var v = voices[i];
				
				msg.postln;
				v.syn.set(name, msg[2]);
			});
		});

		pm.adc_voc.addLevelCommand(this, 'adc_voc');

		nvoices.do({ |i|
			pm.adc_voc.level_(0, i, 0.5);
			pm.adc_voc.level_(1, i, 0.5);
		});
			
	}

	free {
		postln("Engine_Vocer: freeing");
		voices.do({ |v| v.free; });
		pm.do({ |k, m| m.free; });
	}
}
