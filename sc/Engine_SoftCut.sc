// a sample capture / playback matrix
Engine_SoftCut : CroneEngine {
	classvar nbuf = 4; // count of buffes and "fixed" voices
	classvar nvfloat = 4; // count of "floating" voices
	classvar nvoices = 8; // total number of voices
	classvar bufdur = 64.0;

	classvar vfloatIdx; // array of indices for floating voices
	classvar vfixIdx; // array of indices for fixed voices

	classvar commands;

	var <bus; // busses
	var <buf; // buffers
	var <syn; // synths
	var <gr; // groups
	var <pm; // patch matrix
	var <rec; // recorders

	var <voices; // array of voices

	*initClass {
		// lower indices address the floating voices,
		vfloatIdx = Array.series(nvfloat);
		// upper indices are the fixed voices
		vfixIdx =Array.series(nbuf, nvfloat);
	}

	*new { arg context, doneCallback;
		^super.new(context, doneCallback);
	}

	free {
		voices.do({ arg voice; voice.free; });
		buf.do({ |b| b.free; });
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
		var startsamp, endsamp, samps, newbuf;
		startsamp = start * context.server.sampleRate;
		endsamp = end * context.server.sampleRate;
		samps = endsamp - startsamp;
		newbuf = Buffer.alloc(context.server, samps);
		// FIXME: this should be asynchronous
		buf[i].copyData(newbuf, 0, startsamp, samps);
		// any voices using this buffer need to be reassigned
		voices.do({ arg v;
			if(v.buf == buf[i], {
				v.buf = newbuf;
			});
		});
		buf[i].free;
		buf[i] = newbuf;
	}

	// disk read
	readBuf { arg i, path;
		if(buf[i].notNil, {
			/* FIXME:
			 we'd like to automatically trim the buffer to the filesize.
			 so we have to:
			- allocate and read a new buffer.
			[then, asynchronously:]
			- tell any voices using the old buffer to use the new buffer instead
			- change the buffer array to point at the new buffer
			- free the old buffer
			*/
			// (something like trimBuf method)
			buf[i].readChannel(path, channels:[0]);
		});
	}

	// disk write
	writeBuf { arg i, path;
		if(buf[i].notNil, {
			buf[i].write(path, "wav");
		});
	}

	setBuf { arg vidx, bidx;
		if(vidx < nvfloat && bidx < nbuf, {
			voices[vidx].buf_(buf[bidx]);
		});
	}


	playRecLevel { |srcId, dstId, level| pm.pb_rec.level_(srcId, dstId, level); }
	adcRecLevel { |srcId, dstId, level| pm.adc_rec.level_(srcId, dstId, level); }
	playDacLevel { |srcId, dstId, level| pm.pb_dac.level_(srcId, dstId, level); }

	alloc {
		var com;
		var bus_pb_idx; // tmp collection of playback bus indices
		var bus_rec_idx;
		var bufcon;
		var s = context.server;

		postln("SoftCut: init routine");

		//--- groups
		gr = Event.new;
		gr.pb = Group.new(context.xg, addAction:\addToTail);
		gr.rec = Group.after(context.ig);
		// phase bus per voice (output)
		bus = Event.new;

		s.sync;

		//--- buffers

		postln("SoftCut: allocating buffers");

		buf = Array.fill(nvoices, { arg i;
			Buffer.alloc(s, s.sampleRate * bufdur, completionMessage: {
			})
		});

		s.sync;

		//--- busses
		bus.adc = context.in_b;
		// FIXME? not sure about the peculiar arrangement of dual mono in / stereo out.
		// FIXME: oh! actually just use array of panners, instead of output patch matrix.
		// here we convert  output bus to a mono array
		bus.dac = Array.with( Bus.newFrom(context.out_b, 0), Bus.newFrom(context.out_b, 1));
		bus.rec = Array.fill(nvoices, { Bus.audio(s, 1); });
		bus.pb = Array.fill(nvoices, { Bus.audio(s, 1); });

		//-- voices
		voices = Array.fill(nvoices, { |i|
			// 	arg server, target, buf, in, out;
			SoftCutVoice.new(s, context.xg, buf[i], bus.rec[i].index, bus.pb[i].index);
		});

		vfloatIdx.do({ arg idx, i;
			voices[idx].buf_(buf[i]);
			voices[idx].syn.set(\phase_att_in, voices[vfixIdx[i]].phase_b.index);
			voices[idx].syn.set(\phase_att_bypass, 0.0);
		});

		vfixIdx.do({ arg idx, i;
			voices[idx].buf_(buf[i]);
			voices[idx].syn.set(\phase_att_bypass, 1.0);
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

		nvoices.do({ arg i;
			this.addPoll(("phase_" ++ (i+1)).asSymbol, {
			 	var val = voices[i].phase_b.getSynchronous / context.server.sampleRate;
				postln("phase: " ++ val);
				val
			});
			this.addPoll(("phase_norm_" ++ (i+1)).asSymbol, {
				voices[i].phase_b.getSynchronous / (voices[i].buf.duration * context.server.sampleRate);
			});
		});


		s.sync;
	}

	addCommands {


		var com = [

			//-- voice functions
			[\start, \i, {|msg| msg.postln; voices[msg[1]-1].start; }],
			[\stop, \i, {|msg| voices[msg[1]-1].stop; }],
			[\reset, \i, {|msg| voices[msg[1]-1].reset; }],
			[\set_buf, \ii, {|msg| setBuf(msg[1]-1, msg[2]-1); }],

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
			[\adc_rec, \iif, { |msg|
				postln(["Engine_SoftCut: adc_rec", msg]);
				pm.adc_rec.level_(msg[1]-1, msg[2]-1, msg[3]); }],
			// level from given playback channel to given recorder
			[\play_rec, \iif, { |msg| pm.pb_rec.level_(msg[1]-1, msg[2]-1, msg[3]); }],
			// level from given playback channel to given DAC channel
			[\play_dac, \iif, { |msg| pm.pb_dac.level_(msg[1]-1, msg[2]-1, msg[3]); }],

			//--- buffers
			// read named soundfile to given buffer
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
			//			postln("adding command: " ++ comarr);
			this.addCommand(comarr[0], comarr[1], comarr[2]);
		});

	}

}
