GrainDelayVoice {

	var <buf;
	var <syn;
	var <sync_trig_b;
	var <grain_trig_b;

	*initClass {
		StartUp.add {
			var grainDelayDef = SynthDef.new(\grainDelay, {
				arg buf,
				in=0, out=0, amp=0.25,

				in_grain_trig=0,  // trigger input to restart grain pulses
				in_sync_trig=0,  // trigger input to set grain pos to delay value

				grain_pulse_rate=8.0,     // rate of new grain pulses
				grain_pos_rate=1.0, // rate of new grain position scrub
				delay = 1.0,   // delay time value
				pre_record = 0.0, // pre-record value
				loop_start = 0, // loop start point
				loop_end = -1, // loop endpoint. if <0, use the buffer's duration

				// grain parameters
				grain_rate=1.0, grain_dur = 1.0, grain_pan=0, grain_env_buf = -1;

				var input;
				var bufdur;
				var wr_phase, wr;
				var gr, gr_trig, gr_pos_reset, gr_pos;


				input = In.ar(in);
				bufdur = BufDur.kr(buf);
				loop_end = (loop_end < 0).if(bufdur, loop_end);

				wr_phase = Phasor.ar(0, 1, loop_start * SampleRate.ir, loop_end * SampleRate.ir);
				wr = BufWrPre.ar(input, buf, wr_phase, K2A.ar(pre_record));
				// wr_phase.poll;

				gr_pos_reset = ((wr_phase * SampleDur.ir) - delay).wrap(loop_start, loop_end);
				gr_pos = Phasor.kr(InTrig.kr(in_sync_trig), ControlDur.ir * grain_pos_rate,
				loop_start, loop_end, gr_pos_reset);
				gr_pos = gr_pos.max(loop_start).min(loop_end - (grain_dur * grain_rate));
				gr_pos = gr_pos / bufdur;

				// hm, kr phasor doesn't work as trigger, b/c zero isn't always hit on wrap...
				// gr_trig = Phasor.kr(trig_reset, trig_hz, 0, ControlRate.ir);
				/// gr_trig = Phasor.ar( InTrig.kr(in_grain_trig), grain_pulse_rate, 0, SampleRate.ir);
				/// arg...
				gr_trig = Slope.ar( Phasor.ar( InTrig.kr(in_grain_trig), grain_pulse_rate, 0, SampleRate.ir));
				gr = GrainBuf.ar(numChannels:2, trigger:gr_trig, dur:grain_dur, sndbuf:buf,
					rate:grain_rate, pos:gr_pos,  interp:2,
					pan:grain_pan, envbufnum:grain_env_buf);

				Out.ar(out, gr * amp);

			});

			CroneDefs.add(grainDelayDef);
		}
	}


	*new { arg server, in, out, target, bufDur=128;
		^super.new.init(server, in, out, target, bufDur);
	}

	init { arg server, in, out, target, bufDur;
		grain_trig_b = Bus.control(server, 1);
		sync_trig_b = Bus.control(server, 1);

		buf = Buffer.alloc(server, bufDur * server.sampleRate, 1, {
			arg thebuf;
			postln(thebuf);
			syn = Synth.new(\grainDelay, [
				\buf, thebuf.bufnum,
				\in, in,
				\out, out,
				\in_grain_trig, grain_trig_b.index,
				\in_sync_trig, sync_trig_b.index,
			], target:target);
		});
	}

	free {
		postln("GrainDelayVoice.free()");
		buf.free;
		syn.free;
		sync_trig_b.free;
		grain_trig_b.free;
	}

	// triggers
	trig_grain { grain_trig_b.set(1); }
	trig_sync { postln("grain trig_sync"); sync_trig_b.set(1); }

	// setters
	
	amp_ { arg x; syn.set(\amp, x); }
	
	grain_rate_ { arg x; syn.set(\grain_rate, x); }
	grain_pulse_rate_ { arg x; syn.set(\grain_pulse_rate, x); }
	grain_dur_ { arg x; syn.set(\grain_dur, x); }
	grain_pos_rate { arg x; syn.set(\grain_pos_rate, x); }
	grain_pan_ { arg x; syn.set(\grain_pan, x); }
	grain_env_buf_ { arg x; syn.set(\grain_env_buf, x); }

	delay_ { arg x; postln("grain delay_(" ++ x ++ ")"); syn.set(\delay, x); }
	pre_record_ { arg x; syn.set(\pre_record, x); }
	loop_start_ { arg x; syn.set(\loop_start, x); }
	loop_end_ { arg x; syn.set(\loop_end, x); }
}