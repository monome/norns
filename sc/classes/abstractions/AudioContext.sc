// boilerplate audio processing class
// sets up input, output, and simple analysis for stereo enivronment
AudioContext {

	var <>server;
	// input, process, output groups
	var <>ig, <>xg, <>og;
	// input, output busses
	var <>in_b, <>out_b;
	// analysis busses
	var <>amp_in_b, <>amp_out_b, <>pitch_in_b;
	// input, output, monitor synths
	var <>in_s, <>out_s, <>mon_s;
	// analysis synths
	var <>amp_in_s, <>amp_out_s, <>pitch_in_s;

	// polls available in base context
	var <pollNames;

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

		postln("AudioContext: in_b[0] index: " ++ in_b[0].index);
		postln("AudioContext: in_b[1] index: " ++ in_b[1].index);
		postln("AudioContext: out_b index (stereo): " ++ out_b.index);

		amp_in_b = Array.fill(2, { Bus.control(server, 1); });
		amp_out_b = Array.fill(2, { Bus.control(server, 1); });

		// each pitch bus is 2 channels:  [freq, clarity]
		pitch_in_b = Array.fill(2, { Bus.control(server, 2); });

		//---- routing synths

		in_s = Array.fill(2, { |i|
			Synth.new(\adc, [\in, i, \out, in_b[i].index], ig);
		});

		out_s = Synth.new(\patch_stereo, [\in, out_b.index, \out, 0], og);

		mon_s = Array.fill(2, { |i|
			Synth.new(\patch_pan,
				[\in, in_b[i].index, \out, out_b.index, \level, 0],
				ig, \addAfter
			);
		});

		//---- analysis synths

		amp_in_s = Array.fill(2, { |i|
			Synth.new(\amp_env,
				[\in, in_b[i].index, \out, amp_in_b[i].index],
				ig, \addAfter
			);
		});

		amp_out_s = Array.fill(2, { |i|
			Synth.new(\amp_env,
				//[\in, out_b.index + i, \out, amp_out_b[i].index],
				[\in, i, \out, amp_out_b[i].index],
				og, \addAfter
			);
		});

		pitch_in_s = Array.fill(2, { |i|
			Synth.new(\pitch,
				[\in, in_b[i].index, \out, pitch_in_b[i].index],
				ig, \addAfter
			);
		});

		this.initPolls();

	}

	registerPoll  { arg name, func, dt=0.1;
		pollNames.add(name);
		CronePollRegistry.register(name, func, dt);
	}

	initPolls {
		postln("AudioContext: initPolls");
		this.registerPoll(\amp_in_l, { amp_in_b[0].getSynchronous(); });
		this.registerPoll(\amp_in_r, { amp_in_b[1].getSynchronous(); });
		this.registerPoll(\amp_out_l, { amp_out_b[0].getSynchronous(); });
		this.registerPoll(\amp_out_r, { amp_out_b[1].getSynchronous(); });
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
		pitch_in_s.do({ |syn| syn.run(true); });
	}

	pitchOff {
		pitch_in_s.do({ |syn| syn.run(false); });
	}

    outLevel { arg level=1.0;
        out_s.set(\level, level);
    }

}
