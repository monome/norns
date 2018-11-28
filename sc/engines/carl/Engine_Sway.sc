Engine_Sway : CroneEngine {

	*new { arg context, doneCallback;
		^super.new(context, doneCallback);
	}

	alloc {
		NornsSway(\sway);
		NornsSway(\sway).input.set(\chan, context.in_b[0].index);
		NornsSway(\sway).analysis_input.set(\chan, context.in_b[0].index);
		NornsSway(\sway).output.play(context.out_b.index, 2, context.xg, addAction: \addToHead);
		NornsSway(\sway).verbose = false;
		
		this.addCommand("amp_thresh", "f", {|msg|
			NornsSway(\sway).amp_thresh = msg[1];
		});
		
		this.addCommand("clarity_thresh", "f", {|msg|
			NornsSway(\sway).clarity_thresh = msg[1];
		});
		
		this.addCommand("density_thresh", "f", {|msg|
			NornsSway(\sway).density_thresh = msg[1];
		});

		this.addCommand("verbose", "i", {|msg|
			if(msg[1] == 1, {
			NornsSway(\sway).verbose = true;
			}, {
			NornsSway(\sway).verbose = false;
			});
		});
		
		this.addCommand("polarity", "i", {|msg|
			if(msg[1] == 1, {
			NornsSway(\sway).polarity = false;
			NornsSway(\sway).change_polarity;
			}, {
			NornsSway(\sway).polarity = true;
			NornsSway(\sway).change_polarity;
			});
		});

		this.addCommand("fadetime", "i", {|msg|
			NornsSway(\sway).fade_time(msg[1]);
		});

		this.addCommand("timelimit", "i", {|msg|
			NornsSway(\sway).timelimit = msg[1];
		});
		
		this.addCommand("map_quadrant", "ii", {|msg|
			NornsSway(\sway).map_quadrant(msg[1], msg[2]);
		});

		this.addCommand("analysis_on", "i", {|msg|
			if(msg[1] == 1, {
			NornsSway(\sway).analysis_on = true;
			}, {
			NornsSway(\sway).analysis_on = false;
			});
		});
		
		this.addCommand("tracker_on", "i", {|msg|
			if(msg[1] == 1, {
			NornsSway(\sway).tracker_on = true;
			}, {
			NornsSway(\sway).tracker_on = false;
			});
		});
		
		this.addCommand("silence", "i", {|msg|
			NornsSway(\sway).silence;
		});
		
		this.addCommand("reverb", "i", {|msg|
			NornsSway(\sway).reverb;
		});

		this.addCommand("ampmod", "i", {|msg|
			NornsSway(\sway).ampmod;
		});

		this.addCommand("delay", "i", {|msg|
			NornsSway(\sway).delay;
		});

		this.addCommand("filter", "i", {|msg|
			NornsSway(\sway).filter;
		});

		this.addCommand("freeze", "i", {|msg|
			NornsSway(\sway).freeze;
		});

		this.addCommand("pitchbend", "i", {|msg|
			NornsSway(\sway).pitchbend;
		});

		this.addCommand("cascade", "i", {|msg|
			NornsSway(\sway).cascade;
		});
		
		this.addPoll("x_coord", {NornsSway(\sway).xy[0]});
		this.addPoll("y_coord", {NornsSway(\sway).xy[1]});

		this.addPoll("avg_amp", {var amp1, amp30; #amp1, amp30 = NornsSway(\sway).amplitude.bus.getnSynchronous(2); amp30;});

		this.addPoll("avg_onsets", {var onsets1, onsets30; #onsets1, onsets30 = NornsSway(\sway).onsets.bus.getnSynchronous(2);onsets30;});

		this.addPoll("avg_clarity", {var clarity1, clarity30; #clarity1, clarity30 = NornsSway(\sway).clarity.bus.getnSynchronous(2);clarity30;});

		this.addPoll("current_processing", {NornsSway(\sway).quadrant_names[(NornsSway(\sway).quadrant[0])]});
		
		this.addPoll("current_quadrant", {NornsSway(\sway).quadrant[0]});
		
		this.addPoll("quadrant0", {NornsSway(\sway).quadrant_names[0]});
		this.addPoll("quadrant1", {NornsSway(\sway).quadrant_names[1]});
		this.addPoll("quadrant2", {NornsSway(\sway).quadrant_names[2]});
		this.addPoll("quadrant3", {NornsSway(\sway).quadrant_names[3]});
		this.addPoll("quadrant4", {NornsSway(\sway).quadrant_names[4]});

	}

	free {
		NornsSway(\sway).end;
	}
}