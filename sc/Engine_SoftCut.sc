// a sample capture / playback matrix
Engine_SoftCut : CroneEngine {
	classvar nvoices = 4; // total number of voices
	classvar bufdur = 2048; // 64 * 32, >30min

	classvar commands;

	var <bus; // busses
	var <buf; // buffers
	var <syn; // synths
	var <gr; // groups
	var <pm; // patch matrix

	var <voices; // array of voices (r/w heads)

	*new { arg context, doneCallback;
		^super.new(context, doneCallback);
	}

	alloc {
		var com;
		var bus_pb_idx; // tmp collection of playback bus indices
		var bus_rec_idx;
		var bufcon;
		var s = context.server;

		postln("SoftCut: init routine");

		//--- groups
		gr = Event.new;
		gr.rw = Group.new(context.xg, addAction:\addToTail);
		gr.rec = Group.after(context.ig);


		s.sync;

		//--- buffers

		postln("SoftCut: allocating buffers");


		buf = Buffer.alloc(s, s.sampleRate * bufdur);

		s.sync;

		//--- busses
		bus = Event.new;
		bus.adc = context.in_b;

		// here we convert  output bus to a mono array
		bus.dac = Array.with( Bus.newFrom(context.out_b, 0), Bus.newFrom(context.out_b, 1));
		bus.rec = Array.fill(nvoices, { Bus.audio(s, 1); });
		bus.pb = Array.fill(nvoices, { Bus.audio(s, 1); });

		//-- voices
		voices = Array.fill(nvoices, { |i|
			var v = SoftCutVoice.new(s, context.xg, buf, bus.rec[i].index, bus.pb[i].index);
			s.sync;
			if(i == (nvoices-1), {
				voices[i].syn.set(\phase_att_bypass, 1.0);
			}, {
				v.syn.set(\phase_att_in, voices[nvoices-1].phase_b.index);
				v
			});
		});

		//--- patch matrices
		bus_pb_idx = bus.pb.collect({ |b| b.index });
		bus_rec_idx = bus.rec.collect({ |b| b.index });
		pm = Event.new;

		postln("softcut: in->rec patchmatrix");

		// input -> record
		pm.adc_rec = PatchMatrix.new(
			server:s, target:gr.rec, action:\addToTail,
			in: bus.adc.collect({ |b| b.index }),
			out: bus_rec_idx,
			feedback:false
		);
		postln("softcut: pb->out patchmatrix");

		// playback -> output
		pm.pb_dac = PatchMatrix.new(
			server:s, target:gr.pb, action:\addAfter,
			in: bus_pb_idx,
			out: bus.dac.collect({ |b| b.index }),
			feedback:false
		);

		// playback -> record
		postln("softcut: pb->rec patchmatrix");
		pm.pb_rec = PatchMatrix.new(
			server:s, target:gr.rec, action:\addBefore,
			in: bus_pb_idx,
			out: bus_rec_idx,
			feedback:true
		);

		this.addCommands;

		//--- polls
		nvoices.do({ arg i;
			this.addPoll(("phase_" ++ (i+1)).asSymbol, {
				voices[i].phase_b.getSynchronous / context.server.sampleRate;
			});

			this.addPoll(("phase_norm_" ++ (i+1)).asSymbol, {
				voices[i].phase_b.getSynchronous / (voices[i].buf.duration * context.server.sampleRate);
			});
			this.addPoll(("phase_loop_" ++ (i+1)).asSymbol, {
				voices[i].phase_b.getSynchronous / (voices[i].buf.duration * context.server.sampleRate);
			});
		});
	}

	free {
		voices.do({ arg voice; voice.free; });
		buf.free;
		bus.do({ arg bs; bs.do({ arg b; b.free; }); });
		pm.do({ arg p; p.free; });
		super.free;
	}

	//---  buffer and routing methods

	// clear a buffer
	clearBuf { arg i;
		buf[i].zero;
	}

	// normalize buffer to given max
	normalizeBuf { arg i, x;
		buf[i].normalize(x);
	}

	// destructive trim
	trimBuf { arg i, start, end;
		BufUtil.trim (buf[i], start, end, {
			arg newbuf;
			voices.do({ arg v;
				if(v.buf == buf[i], {
					v.buf = newbuf;
				});
			});
			buf[i] = newbuf;
		});
	}

	// disk read (replacing)
	replaceBuf { arg i, path;
		if(buf[i].notNil, {
			BufUtil.readChannel(buf, path, {
				arg newbuf;
				voices.do({ arg v;
					if(v.buf == buf[i], {
						v.buf = newbuf;
					});
				});
				buf[i].free;
			});
		});
	}

	// disk read (copying over current contents)
	readBuf { arg i, path, start, dur;
		if(buf[i].notNil, {
			BufUtil.copyChannel(buf[i], path, start:start, dur:dur);
		});
	}

	// disk write
	writeBuf { arg i, path, start, dur;
		if(buf[i].notNil, {
			BufUtil.write(buf[i], path, start:start, dur:dur);
		});
	}


	syncVoice { arg src, dst, offset;
		voices[dst].syn.set(\pos, voices[src].phase_b.getSynchronous / context.sampleRate + offset);
		voices[dst].reset;
	}

	// helpers
	playRecLevel { |srcId, dstId, level| pm.pb_rec.level_(srcId, dstId, level); }
	adcRecLevel { |srcId, dstId, level| pm.adc_rec.level_(srcId, dstId, level); }
	playDacLevel { |srcId, dstId, level| pm.pb_dac.level_(srcId, dstId, level); }

	addCommands {

		var com = [

			//-- voice functions
			[\start, \i, {|msg| msg.postln; voices[msg[1]-1].start; }],
			[\stop, \i, {|msg| voices[msg[1]-1].stop; }],
			[\reset, \i, {|msg| voices[msg[1]-1].reset; }],
			[\sync, \ii, {|msg| syncVoice(msg[1]-1, msg[2]-1) }],

			//-- direct control of synth params
			[\amp, \if, { |msg| voices[msg[1]-1].syn.set(\amp, msg[2]); }],
			[\rec, \if, { |msg| voices[msg[1]-1].syn.set(\rec, msg[2]); }],
			[\pre, \if, { |msg| voices[msg[1]-1].syn.set(\pre, msg[2]); }],
			[\rate, \if, { |msg| voices[msg[1]-1].syn.set(\rate, msg[2]); }],
			[\rate_lag, \if, { |msg| voices[msg[1]-1].syn.set(\ratelag, msg[2]); }],
			[\loop_start, \if, { |msg| voices[msg[1]-1].syn.set(\start, msg[2]); }],
			[\loop_end, \if, { |msg| voices[msg[1]-1].syn.set(\end, msg[2]); }],
			[\pos, \if, { |msg| voices[msg[1]-1].syn.set(\pos, msg[2]); }],
			[\fade, \if, { |msg| voices[msg[1]-1].syn.set(\fade, msg[2]); }],
			[\loop_on, \if, { |msg| voices[msg[1]-1].syn.set(\loop, msg[2]); }],
			[\fade_rec, \if, { |msg| voices[msg[1]-1].syn.set(\fadeRec, msg[2]); }],
			[\fade_pre, \if, { |msg| voices[msg[1]-1].syn.set(\fadePre, msg[2]); }],
			[\rec_on, \if, { |msg| voices[msg[1]-1].syn.set(\recRun, msg[2]); }],
			[\offset, \if, { |msg| voices[msg[1]-1].syn.set(\offset, msg[2]); }],
			[\pre_lag, \if, { |msg| voices[msg[1]-1].syn.set(\preLag, msg[2]); }],
			[\rec_lag, \if, { |msg| voices[msg[1]-1].syn.set(\recLag, msg[2]); }],
			[\env_time, \if, { |msg| voices[msg[1]-1].syn.set(\envTimeScale, msg[2]); }],

			//-- routing
			// level from given ADC channel to given recorder
			[\adc_rec, \iif, { |msg| pm.adc_rec.level_(msg[1]-1, msg[2]-1, msg[3]); }],
			// level from given playback channel to given recorder
			[\play_rec, \iif, { |msg| pm.pb_rec.level_(msg[1]-1, msg[2]-1, msg[3]); }],
			// level from given playback channel to given DAC channel
			[\play_dac, \iif, { |msg| pm.pb_dac.level_(msg[1]-1, msg[2]-1, msg[3]); }],

			//--- buffers
			// read named soundfile to given buffer (overwriting region)
			[\read, \is, { |msg| this.readBuf(msg[1]-1, msg[2]) }],
			// write given buffer to named soundfile
			[\write, \is, { |msg| this.writeBuf(msg[1]-1, msg[2]) }],
			// clear given buffer
			[\clear, \i, { |msg| this.clearBuf(msg[1]-1) }],
			// detructively trim to new start and end
			[\trim, \iff, { |msg| this.trimBuf(msg[1]-1, msg[2], msg[3]) }],
			// normalize given buffer to given maximum level
			[\norm, \if, { |msg| this.normalizeBuf(msg[1]-1, msg[2]) }]
		];

		com.do({ arg comarr;
			this.addCommand(comarr[0], comarr[1], comarr[2]);
		});

	}

}
