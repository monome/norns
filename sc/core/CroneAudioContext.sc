// boilerplate audio processing class
// sets up input, output, and simple analysis for stereo enivronment
CroneAudioContext {

	var <>server;
	// input, process, output groups
	// FIXME: not good naming, use an Event to match engine style
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

	// I/O VU levels are reported on a dedicated OSC address
	var vu_thread;
	var <>vu_dt;


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

		// input 2xmono, output stereo, seems like a "normal" arrangement (?)
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

		out_s = Synth.new(\patch_stereo, [\in, out_b.index, \out, 0, \level, 1.0], og);

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


	inputLevel { arg chan, db; in_s[chan].set(\level, db.dbamp);  }
	outputLevel { arg db; out_s.set(\level, db.dbamp); }

	// control monitor level / pan
	monitorLevel { arg db;
		mon_s.do({|syn| syn.set(\level, db.dbamp) });
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
		postln("AudioContext.pitchOn");
		pitch_in_s.do({ |syn| syn.run(true); });
	}

	pitchOff {
		pitch_in_s.do({ |syn| syn.run(false); });
	}

	registerPoll  { arg name, func, dt=0.1, type=\value;
		pollNames.add(name);
		CronePollRegistry.register(name, func, dt, type);
	}

	// pack low-resolution, log-scaled bus amplitudes
	buildVuBlob {

		var ret = Int8Array.newClear(4);
		ret[0] = ReverseAudioTaper.lookup( amp_in_b[0].getSynchronous );
		ret[1] = ReverseAudioTaper.lookup( amp_in_b[1].getSynchronous );
		ret[2] = ReverseAudioTaper.lookup( amp_out_b[0].getSynchronous );
		ret[3] = ReverseAudioTaper.lookup( amp_out_b[1].getSynchronous );
		^ret;
	}

	initPolls {
		postln("AudioContext: initPolls");
		this.registerPoll(\cpu_peak, { server.peakCPU });
		this.registerPoll(\cpu_avg, { server.avgCPU; });

		this.registerPoll(\amp_in_l, { amp_in_b[0].getSynchronous; });
		this.registerPoll(\amp_in_r, { amp_in_b[1].getSynchronous; });
		this.registerPoll(\amp_out_l, { amp_out_b[0].getSynchronous; });
		this.registerPoll(\amp_out_r, { amp_out_b[1].getSynchronous; });

		this.registerPoll(\pitch_in_l, {
			var pitch, clar;
			#pitch, clar = this.pitch_in_b[0].getnSynchronous(2);
			// postln(["pitch_in_l", pitch, clar]);
			if(clar > 0, { pitch }, {-1});
		});
		this.registerPoll(\pitch_in_r, {
			var pitch, clar;
			#pitch, clar = this.pitch_in_b[1].getnSynchronous(2);
			if(clar > 0, { pitch }, {-1});
		});
	}
}
