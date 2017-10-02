// boilerplate audio processing class
// sets up input, output, and simple analysis for stereo enivronment
AudioContext {

	var <>server;
	// input, process, output groups
	var <>ig, <>xg, <>og;
	// input, output busses
	var in_b, out_b;
	// analysis busses
	var in_amp_b, out_amp_b, pitch_b;
	// input, output, monitor synths
	var in_s, out_s, mon_s;
	// analysis synths
	var in_amp_s, out_amp_s, pitch_s;
	
	*new { arg srv;
		^super.new.init(srv);
	}

	init {
		arg srv;
		server = srv;

		//---- groups
		
		ig = Group.new(server);
		xg = Group.after(ig);
		og = Group.after(xg);

		//---- busses
		
		// input 2xmono, output stereo, seems like a "normal" arrangement
		in_b = Array.fill(2, { Bus.audio(server, 1); });		
		out_b = Bus.audio(server, 2);
		
		in_amp_b = Array.fill(2, { Bus.control(server, 1); });
		out_amp_b = Array.fill(2, { Bus.control(server, 1); });
		
		// each pitch bus is 2 channels:  [freq, clarity]
		pitch_b = Array.fill(2, { Bus.control(server, 2); });


		//---- routing synths
		
		in_s = Array.fill(2, { |i|
			Synth.new(\adc, [\in, i, \out, in_b[i].index], ig);
		});

		out_s = Synth.new(\adc, [\in, out_b.index, \out, 0], og);

		mon_s = Array.fill(2, { |i|
			Synth.new(\patch_pan,
				[\in, in_b[i].index, \out, out_b.index],
				ig, \addAfter
			);
		});

		//---- analysis synths

		in_amp_s = Array.fill(2, { |i|
			Synth.new(\amp_env,
				[\in, in_b[i].index, \out, in_amp_b[i].index],
				ig, \addAfter
			);
		});

		out_amp_s = Array.fill(2, { |i|
			Synth.new(\amp_env,
				[\in, out_b.index + i, \out, out_amp_b[i].index],
				og, \addAfter
			);
		});

		pitch_s = Array.fill(2, { |i|
			Synth.new(\pitch,
				[\in, in_b[i].index, \out, pitch_b[i].index],
				ig, \addAfter
			);
		});

	}

	// control monitor level / pan
	
	setMonitorLevel { arg level;
		mon_s.do({|syn| syn.set(\level, level) });
	}
	
	monitorMono {
		mon_s[0].set(\pan, 0);
		mon_s[1].set(\pan, 0);
	}

	monitorStereo {
		mon_s[0].set(\pan, -1);
		mon_s[1].set(\pan, 1);	
	}

	// toggle monitoring altogether (will cause clicks)
	
	monitorOn {
		mon_s.do({ |syn| syn.run(true); });
	}

	monitorOff {
		mon_s.do({ |syn| syn.run(false); });
	}

	// toggle pitch analysis (save CPU)
	
	pitchOn {
		pitch_s.do({ |syn| syn.run(true); });
	}

	pitchOff {
		pitch_s.do({ |syn| syn.run(false); });
	}

	
}