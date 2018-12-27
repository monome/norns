Engine_Sway : CroneEngine {

	*new { arg context, doneCallback;
		^super.new(context, doneCallback);
	}

	alloc {
		Norns_Sway(\sway);
		Norns_Sway(\sway).input.set(\chan, context.in_b[0].index);
		Norns_Sway(\sway).analysis_input.set(\chan, context.in_b[0].index);
		Norns_Sway(\sway).output.play(context.out_b.index, 2, context.xg, addAction: \addToHead);
		Norns_Sway(\sway).verbose = false;
		//Norns_Sway(\sway).analysis_on = true;
		
		this.addCommand("amp_thresh", "f", {|msg|
			Norns_Sway(\sway).amp_thresh = msg[1];
		});
		
		this.addCommand("clarity_thresh", "f", {|msg|
			Norns_Sway(\sway).clarity_thresh = msg[1];
		});
		
		this.addCommand("density_thresh", "f", {|msg|
			Norns_Sway(\sway).density_thresh = msg[1];
		});

		this.addCommand("verbose", "i", {|msg|
			if(msg[1] == 1, {
			Norns_Sway(\sway).verbose = true;
			}, {
			Norns_Sway(\sway).verbose = false;
			});
		});
		
		this.addCommand("polarity", "i", {|msg|
			if(msg[1] == 1, {
			Norns_Sway(\sway).polarity = false;
			Norns_Sway(\sway).change_polarity;
			}, {
			Norns_Sway(\sway).polarity = true;
			Norns_Sway(\sway).change_polarity;
			});
		});

		this.addCommand("fadetime", "i", {|msg|
			Norns_Sway(\sway).fade_time(msg[1]);
		});

		this.addCommand("timelimit", "i", {|msg|
			Norns_Sway(\sway).timelimit = msg[1];
		});
		
		this.addCommand("map_quadrant", "ii", {|msg|
			Norns_Sway(\sway).map_quadrant(msg[1], msg[2]);
		});

		this.addCommand("analysis_on", "i", {|msg|
			if(msg[1] == 1, {
			Norns_Sway(\sway).analysis_on = true;
			}, {
			Norns_Sway(\sway).analysis_on = false;
			});
		});
		
		this.addCommand("tracker_on", "i", {|msg|
			if(msg[1] == 1, {
			Norns_Sway(\sway).tracker_on = true;
			}, {
			Norns_Sway(\sway).tracker_on = false;
			});
		});
		
		this.addCommand("panning_on", "i", {|msg|
			if(msg[1] == 1, {
			Norns_Sway(\sway).panstereo;
			}, {
			Norns_Sway(\sway).pancenter;
			});
		});
		
		this.addCommand("silence", "i", {|msg|
			Norns_Sway(\sway).silence;
		});
		
		this.addCommand("reverb", "i", {|msg|
			Norns_Sway(\sway).reverb;
		});

		this.addCommand("ampmod", "i", {|msg|
			Norns_Sway(\sway).ampmod;
		});

		this.addCommand("grains", "i", {|msg|
			Norns_Sway(\sway).grains;
		});

		this.addCommand("delay", "i", {|msg|
			Norns_Sway(\sway).delay;
		});
		
		this.addCommand("granular", "i", {|msg|
			Norns_Sway(\sway).granular;
		});

		this.addCommand("filter", "i", {|msg|
			Norns_Sway(\sway).filter;
		});

		this.addCommand("freeze", "i", {|msg|
			Norns_Sway(\sway).freeze;
		});

		this.addCommand("pitchbend", "i", {|msg|
			Norns_Sway(\sway).pitchbend;
		});

		this.addCommand("cascade", "i", {|msg|
			Norns_Sway(\sway).cascade;
		});
		
		this.addPoll("x_coord", {Norns_Sway(\sway).xy[0]});
		this.addPoll("y_coord", {Norns_Sway(\sway).xy[1]});

		this.addPoll("avg_amp", {var amp1, amp30; #amp1, amp30 = Norns_Sway(\sway).amplitude.bus.getnSynchronous(2); amp30;});

		this.addPoll("avg_onsets", {var onsets1, onsets30; #onsets1, onsets30 = Norns_Sway(\sway).onsets.bus.getnSynchronous(2);onsets30;});

		this.addPoll("avg_clarity", {var clarity1, clarity30; #clarity1, clarity30 = Norns_Sway(\sway).clarity.bus.getnSynchronous(2);clarity30;});

		this.addPoll("current_processing", {Norns_Sway(\sway).quadrant_names[(Norns_Sway(\sway).quadrant[0])]});
		
		this.addPoll("current_quadrant", {Norns_Sway(\sway).quadrant[0]});
		
		this.addPoll("quadrant0", {Norns_Sway(\sway).quadrant_names[0]});
		this.addPoll("quadrant1", {Norns_Sway(\sway).quadrant_names[1]});
		this.addPoll("quadrant2", {Norns_Sway(\sway).quadrant_names[2]});
		this.addPoll("quadrant3", {Norns_Sway(\sway).quadrant_names[3]});
		this.addPoll("quadrant4", {Norns_Sway(\sway).quadrant_names[4]});

	}

	free {
		Norns_Sway(\sway).end;
	}
}