/*
	an audio patch matrix
	connects arbitrary input and output bus arrays
*/

PatchMatrix {
	var <gr; // collection of groups
	var <syn; // collection of synths
	var <numInputs;
	var <numOutputs;

	*initClass {
		StartUp.add {
			CroneDefs.add(
				SynthDef.new(\patch_mono_gate_pause, {
					arg in, out, level, gate=0, time=0.1;
					var ampenv, input;
					ampenv = EnvGen.kr(Env.asr(1, 1, 1),
						timeScale:time, gate:gate, doneAction:1);
					input = In.ar(in);
					Out.ar(out, input * ampenv * level);
				})
			);
			
			CroneDefs.add(
				SynthDef.new(\patch_mono_gate_pause_fb, {
					arg in, out, level, gate=0, time=0.1;
					var ampenv, input;
					ampenv = EnvGen.kr(Env.asr(1, 1, 1),
						timeScale:time, gate:gate, doneAction:1);
					input = InFeedback.ar(in);
					Out.ar(out, input * ampenv * level);
				})
			);
		}
	}
	
	// arg 1: server
	// arg 2: array of input bus indices
	// arg 3: array of output bus indices
	// arg 4: feedback flag, if true input will be read from previous block
	// arg 5: target node
	// arg 6: add action
	*new { arg server, in, out,
		feedback = false, target=nil, action=\addToTail;
		^super.new.init(server, in, out, feedback, target, action);
	}

	free {
		gr.free; // frees all synth
	}

	init { arg srv, in, out, fb, target, action;
		var patchdef;
		patchdef = if(fb,
			{\patch_mono_gate_pause_fb},
			{\patch_mono_gate_pause}
		);
		
		if(target.isNil, { target = srv });
		gr = Group.new(target, action);
		numInputs = in.size;
		numOutputs = out.size;
		syn = in.collect({ arg in, i;
			out.collect({ arg out, j;
				Synth.new( patchdef, [
					\in, in, \out, out, \level, 0.0
				], gr, \addToTail)
			})
		})
	}	

	level_ { arg in, out, val;
		postln(["PatchMatrix: level_ ", in, out, val]);
		syn[in][out].set(\level, val);
		if(val > 0, {
			syn[in][out].run(true);
			syn[in][out].set(\gate, 1);
		}, {
			syn[in][out].set(\gate, 0);
		});
	}

	// convenience method to create a command on a given CroneEngine, affecting a patch point level
	addLevelCommand { arg engine, name;
		engine.addCommand(name, "iif", {
			arg msg;
			var i = msg[1]-1;
			var j = msg[2]-1;
			var v = msg[3];
			if(i>0 && j>0 && i<numInputs && j<numOutputs, {
				syn[i][j].set(\level, v.min(1.0).max(0.0));
			});
		});
	}
}