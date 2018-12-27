// a sample capture / playback matrix
Engine_SoftCut : CroneEngine {
	classvar nvoices = 4; // total number of voices
	classvar bufDur = 480; // 8 minutes +clear
	
	classvar commands;

	var <bus; // busses
	var <buf; // buffer
	var <gr; // groups
	var <pm; // patch matrix

	var <voices; // array of voices (r/w heads)
	var <trig_s; // array of trigger synths
	var <adc_s;
	var <dac_s;
	var <phase_quant_poll;
	var <buf_dur_poll;

	*new { arg context, doneCallback;
		^super.new(context, doneCallback);
	}

	*initClass {
		StartUp.add {
			CroneDefs.add(
				// send trigger when quantized KR signal changes
				SynthDef.new(\quant_trig,  {
					arg in, quant=48000, offset=0, id=0;
					var sgl, tr;
					//sgl = In.kr(in).round(quant);
					sgl = ((In.kr(in)+offset) / quant).floor * quant;
					tr = Changed.kr(sgl);
					SendTrig.kr(tr, id, sgl);
				});
			);
		}
	}

	alloc {
		var com;
		var bus_pb_idx; // tmp collection of playback bus indices
		var bus_rec_idx;
		var bufcon;
		var s = context.server;
		var start_stride = (bufDur - 2.0) / nvoices;
		var bufFrames;
		
		postln("SoftCut: init routine");

		//--- groups
		gr = Event.new;
		gr.pb = Group.new(context.xg, addAction:\addToTail);
		gr.rec = Group.after(context.ig);
		gr.adc = Group.before(gr.rec);
		gr.dac = Group.after(gr.pb);
		gr.voices = ParGroup.new(context.xg, addAction:\addToHead);

		s.sync;

		//--- buffers		
		bufFrames = (s.sampleRate * bufDur).nextPowerOfTwo;
		bufDur = bufFrames / s.sampleRate;
		postln("bufFrames: " ++ bufFrames);
		postln("bufDur: " ++ bufDur);
		buf = Buffer.alloc(s, bufFrames, 1);

		s.sync;

		postln(buf);

		//--- busses
		postln("busses...");
		bus = Event.new;	
		bus.adc = Array.fill(2, { Bus.audio(s, 1) });
		bus.dac = Array.fill(2, { Bus.audio(s, 1) });
		bus.rec = Array.fill(nvoices, { Bus.audio(s, 1); });
		bus.pb = Array.fill(nvoices, { Bus.audio(s, 1); });

		//-- voices
		postln("voices...");
		voices = Array.fill(nvoices, { |i|
			SoftCutVoice.new(s, gr.voices, buf, bus.rec[i].index, bus.pb[i].index);
		});
		s.sync;
			// by default, place the first n-1 voices equally in the buffer, without overlap
			voices.do({ arg voice, i;
				if(i == (nvoices-1), {
					voice.syn.set(\phase_att_bypass, 1.0);
					voice.syn.set(\start, 0);
					voice.syn.set(\end, buf.duration);
				}, {
					voice.syn.set(\start, i * start_stride + 1.0);
					voice.syn.set(\pos, i * start_stride);
					voice.syn.set(\end, (i+1)*start_stride);
					voice.syn.set(\phase_att_bypass, 0.0);
					voice.syn.set(\phase_att_in, voices[nvoices-1].phase_b.index);
				});
			});

		//--- patch matrices
		postln("patch matrices...");
		bus_pb_idx = bus.pb.collect({ |b| b.index });
		bus_rec_idx = bus.rec.collect({ |b| b.index });
		pm = Event.new;

		// input -> record
		postln("(in->rec)");
		pm.adc_rec = PatchMatrix.new(
			server:s, target:gr.rec, action:\addToTail,
			in: bus.adc.collect({ |b| b.index }),
			out: bus_rec_idx,
			feedback:false
		);

		// playback -> output
		postln("(pb->out)");
		pm.pb_dac = PatchMatrix.new(
			server:s, target:gr.pb, action:\addAfter,
			in: bus_pb_idx,
			out: bus.dac.collect({ |b| b.index }),
			feedback:false
		);

		// playback -> record
		postln("(pb->rec)");
		pm.pb_rec = PatchMatrix.new(
			server:s, target:gr.rec, action:\addBefore,
			in: bus_pb_idx,
			out: bus_rec_idx,
			feedback:true
		);

		// make our own I/O busses and synths... shouldn't be necessary but weird things seem to happen otherwise.
		postln("adc synths");
		adc_s = Array.fill(2, { |i|
			Synth.new(\patch_mono, [\in, context.in_b[i].index, \out, bus.adc[i].index], gr.adc);
		});

		postln("dac synths");
		dac_s = Array.fill(2, { |i|
			Synth.new(\patch_mono, [\in, bus.dac[i].index, \out, context.out_b.index + i], gr.dac);
		});

		this.addCommands;
		this.addOscTriggers;

		//--- polls
		postln("polls...");
		nvoices.do({ arg i;
			Post << "adding polls " << i << "\n";
			this.addPoll(("phase_" ++ (i+1)), {
				voices[i].phase_b.getSynchronous / context.server.sampleRate;
			});

			this.addPoll(("phase_buf_" ++ (i+1)), {
				voices[i].phase_b.getSynchronous / voices[i].buf.duration;
			});
			this.addPoll(("phase_loop_" ++ (i+1)), {
				voices[i].phase_b.getSynchronous / (voices[i].end - voices[i].start);
			});
		});

		phase_quant_poll = Array.fill(nvoices, { arg i;
			this.addPoll("phase_quant_" ++ (i+1), periodic:false);
		});
		
		buf_dur_poll = this.addPoll("buf_dur", periodic:false);
		buf_dur_poll.update(buf.duration);
	}

	free {
		voices.do({ arg voice; voice.free; });
		buf.free;
		adc_s.do({ arg syn; syn.free; });
		dac_s.do({ arg syn; syn.free; });
		bus.do({ arg arr; arr.do({ arg b; b.free; }) });
		pm.do({ arg p; p.free; });
	}

	//---  buffer and routing methods

	// clear the entire buffer
	clearBuf {
		buf.zero;
	}

    // clear range of buffer
    clearBufRange { arg startFrame, numFrames;
        postln("clear buffer range start " ++ startFrame ++ " len " ++ numFrames);
        buf.fill(startFrame, numFrames, 0.0);
    }

	// normalize buffer to given max
	normalizeBuf { arg x;
		buf.normalize(x);
	}

	// disk read to (copying over current contents)
	readBuf { arg path, start, dur;
		postln("copy channel range; start=" ++ start ++ "; dur=" ++ dur);
		BufUtil.copyChannel(buf, path, start:start, dur:dur);
	}

	// disk write
	writeBuf { arg path, start, dur;
			BufUtil.write(buf, path, start, dur);
	}

	syncVoice { arg src, dst, offset;
		voices[dst].syn.set(\pos, voices[src].phase_b.getSynchronous / context.sampleRate + offset);
		voices[dst].reset;
	}

	// helpers
	playRecLevel { |srcId, dstId, level| pm.pb_rec.level_(srcId, dstId, level); }
	adcRecLevel { |srcId, dstId, level| pm.adc_rec.level_(srcId, dstId, level); }
	playDacLevel { |srcId, dstId, level| pm.pb_dac.level_(srcId, dstId, level); }

	addOscTriggers {

		trig_s = voices.collect({ arg voice, i;
			Synth.new(\quant_trig, [\in, voice.phase_b, \id, i, \quant, 1/16], gr.pb, \addAfter);
		});

		OSCdef(\quant_trig, { arg msg, time;
			var idx = msg[2];
			var val = msg[3];
			phase_quant_poll[idx].update(val);
		}, '/tr', context.server.addr);
	}

	addCommands {

		var com = [

			//-- voice functions
			// start playing/recording
			[\start, \i, {|msg| voices[msg[1]-1].start; }],

			// stop playing/recording
			[\stop, \i, {|msg| voices[msg[1]-1].stop; }],

			// immediately jump to the position indicated by `pos`
			[\reset, \i, {|msg| voices[msg[1]-1].reset; }],

			// immediately set one voice's position to that of another voice, plus offset
			[\sync, \iif, {|msg| syncVoice(msg[1]-1, msg[2]-1, msg[3]); }],

			// set the quantization (rounding) interval for phase reporting on given voice
			// FIXME: clamp this to something reasonable instead of msec?
			[\quant, \if, {|msg| trig_s[msg[1]-1].set(\quant, msg[2].max(0.001)); }],

			// set offset for quantization calculation
			[\quant_offset, \if, {|msg| trig_s[msg[1]-1].set(\offset, msg[2]); }],

			//-- direct control of synth params
			// output amplitude
			[\amp, \if, { |msg| voices[msg[1]-1].syn.set(\amp, msg[2]); }],

			// level of new audio while recording
			[\rec, \if, { |msg| voices[msg[1]-1].syn.set(\rec, msg[2]); }],

			// level of existing audio while recording
			[\pre, \if, { |msg| voices[msg[1]-1].syn.set(\pre, msg[2]); }],

			// playback rate
			[\rate, \if, { |msg| voices[msg[1]-1].syn.set(\rate, msg[2]); }],

			// playback rate lag time
			[\rate_lag, \if, { |msg| voices[msg[1]-1].syn.set(\ratelag, msg[2]); }],

			// pre-level lag time
			[\pre_lag, \if, { |msg| voices[msg[1]-1].syn.set(\preLag, msg[2]); }],

			// record level lag time
			[\rec_lag, \if, { |msg| voices[msg[1]-1].syn.set(\recLag, msg[2]); }],

			// loop start point in seconds
			[\loop_start, \if, { |msg| voices[msg[1]-1].syn.set(\start, msg[2]); }],

			// loop end point in seconds
			[\loop_end, \if, { |msg| voices[msg[1]-1].syn.set(\end, msg[2]); }],

			// position (in seconds) to jump to when reset is triggered
			[\pos, \if, { |msg| voices[msg[1]-1].syn.set(\pos, msg[2]);}],

			// offset in *samples* between record and play
			[\offset, \if, { |msg| voices[msg[1]-1].syn.set(\offset, msg[2]); }],

			// cross-fade time
			[\fade, \if, { |msg| voices[msg[1]-1].syn.set(\fade, msg[2]); }],

			// amount by which crossfade applies to record level
			[\fade_rec, \if, { |msg| voices[msg[1]-1].syn.set(\fadeRec, msg[2]); }],

			// amount by which inverse of crossfade applies to pre-record level
			[\fade_pre, \if, { |msg| voices[msg[1]-1].syn.set(\fadePre, msg[2]); }],

			// toggle looping on/off
			[\loop_on, \if, { |msg| voices[msg[1]-1].syn.set(\loop, msg[2]); }],

			// toggle recording on/off
			[\rec_on, \if, { |msg| voices[msg[1]-1].syn.set(\recRun, msg[2]); }],

			// amplitude envelope time scaling (attack and release)
			[\env_time, \if, { |msg| voices[msg[1]-1].syn.set(\envTimeScale, msg[2]); }],


			//-- routing
			// level from given ADC channel to given recorder
			[\adc_rec, \iif, { |msg| pm.adc_rec.level_(msg[1]-1, msg[2]-1, msg[3]); }],

			// level from given playback channel to given recorder
			[\play_rec, \iif, { |msg| pm.pb_rec.level_(msg[1]-1, msg[2]-1, msg[3]); }],

			// level from given playback channel to given DAC channel
			[\play_dac, \iif, { |msg| pm.pb_dac.level_(msg[1]-1, msg[2]-1, msg[3]); }],


			//--- buffers
			// read named soundfile to buffer region
			[\read, \sff, { |msg| this.readBuf(msg[1], msg[2], msg[3]) }],

			// write buffer region to named soundfile
			[\write, \sff, { |msg| this.writeBuf(msg[1], msg[2], msg[3]) }],

			// clear the entire buffer
			[\clear, '', { |msg| this.clearBuf }],

			// clear buffer range
			[\clear_range, \ii, { |msg| this.clearBufRange(msg[1], msg[2]) }],

			// normalize buffer to given maximum level
			[\norm, \f, { |msg| this.normalizeBuf(msg[1]) }]
			// TODO: normalize range?
		];

		com.do({ arg comarr;
			this.addCommand(comarr[0], comarr[1], comarr[2]);
		});

	}

}
