/*
	an audio patch matrix
	connects arbitrary input and output bus arrays
*/

PatchMatrix {
	var <bus; // collection of busses
	var <gr; // collection of groups
	var <syn; // collection of synths
	var <numInputs;
	var <numOutputs;

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
		gr.free; // frees all synths
		bus.do({ |b| b.free; });
	}

	init { arg srv, in, out, fb, target, action;
		var patchdef;
		patchdef = if(fb, {\patch_mono_fb}, {\patch_mono});
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
	}


	// convenience method to create a command on a given CroneEngine, affecting a patch point level
	addLevelCommand { arg name, engine;
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