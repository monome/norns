// scapes

// granular delay
// oscillator / ringmod bank

Engine_Scapes : CroneEngine {

	classvar <num_grain_del;
	classvar <num_osc;

	var <grain_del; // array of GrainDelayVoices
	var <osc; // array of ScapeOscVoices

	*initClass {
		num_grain_del = 4;
		num_osc = 16;
	}

	*new { arg context, doneCallback;
		^super.new(context, doneCallback);
	}


	alloc {
		var c = Crone.ctx;
		var s = Crone.server;

		grain_del = Array.fill(num_grain_del, {
			GrainDelayVoice.new(s, c.in_b[0], c.out_b, c.xg);
		});

		// trigger a new grain, restarting grain pulses
		this.addCommand(\trig_grain, "i", { arg msg;
			msg.postln;
			grain_del[msg[1]-1].trig_grain(); });
		// set grain playback position to write position, minus delay offset
		this.addCommand(\trig_grain_sync, "i", { arg msg;
			msg.postln;
			grain_del[msg[1]-1].trig_sync(); });
		// set grain output amplitude
		this.addCommand(\grain_amp, "if", { arg msg;
			msg.postln;
			grain_del[msg[1]-1].amp_(msg[2]); });
		// set grain playback rate
		this.addCommand(\grain_rate, "if", { arg msg;
			msg.postln;
			grain_del[msg[1]-1].grain_rate_(msg[2]); });
		// set rate of pulse train triggering new grains
		this.addCommand(\grain_pulse_rate, "if", { arg msg;
			msg.postln;
			grain_del[msg[1]-1].grain_pulse_rate_(msg[2]); });
		// set duration of grains
		this.addCommand(\grain_dur, "if", { arg msg;
			msg.postln;
			grain_del[msg[1]-1].grain_dur_(msg[2]); });
		// set grain position rate 
		this.addCommand(\grain_pos_rate, "if", { arg msg;
			msg.postln;
			grain_del[msg[1]-1].grain_(msg[2]); });
		// set grain pan
		this.addCommand(\grain_pan, "if", { arg msg;
			msg.postln;
			grain_del[msg[1]-1].grain_pan_(msg[2]); });
		// set delay time
		this.addCommand(\grain_delay, "if", { arg msg;
			var gd = grain_del[msg[1]-1];
			msg.postln;
			fork {
				gd.delay_(msg[2]);
				s.sync;
				gd.trig_sync();
			}
		});
		// set pre-record amount (sound-on-sound)
		this.addCommand(\grain_pre_record, "if", { arg msg; 
			msg.postln;
			grain_del[msg[1]-1].pre_record_(msg[2]); });
		// set loop start point
		this.addCommand(\grain_loop_start, "if", { arg msg; 
			msg.postln;
			grain_del[msg[1]-1].loop_start_(msg[2]); });
		// set loop end point
		this.addCommand(\grain_loop_end, "if", { arg msg; 
			msg.postln;
			grain_del[msg[1]-1].loop_end_(msg[2]); });


	}



	free {
		grain_del.do({ |gd| gd.free; });
	}

}

