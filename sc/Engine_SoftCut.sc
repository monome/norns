// a sample capture / playback matrix
Engine_SoftCut : CroneEngine {
	classvar nvfloat=4; // count of "floating" voices (follow any buffer)
	classvar nbuf = 4; // number of buffers (and "fixed voices")
	classvar nvoices = 8; // total voices (fixed + floating)
	classvar bufdur = 64.0;

	classvar commands;

	var <bus; // busses
	var <buf; // buffers
	var <syn; // synths
	var <gr; // groups
	var <pm; // patch matrix
	var <voices;

	*new { arg context, doneCallback;
		^super.new(context, doneCallback);
	}

	free {
		voices.do({ arg v; v.free; });
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
			buf[i].readChannel(path, channels:[0]);
		});
	}

	// disk write
	writeBuf { arg i, path;
		if(buf[i].notNil, {
			buf[i].write(path, "wav");
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
		gr.pb = Group.new(context.xg);
		gr.rec = Group.after(context.ig);
		// phase bus per voice (output)
		bus = Event.new;

		s.sync;

		//--- buffers

		postln("SoftCut: allocating buffers");
		buf = Array.fill(nbuf, { arg i;
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
		bus.rec = Array.fill(nbuf, { Bus.audio(s, 1); });
		bus.pb = Array.fill(nvoices, { Bus.audio(s, 1); });

		//-- voices
		voices = Array.fill(nvoices, { |i|
			// 	arg server, target, buf, in, out;
			SoftCutVoice.new(s, context.xg, buf[i], bus.rec[i].index, bus.pb[i].index);
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
			feedback:true
		);
		postln("softcut: pb->out patchmatrix");
		// playback -> output
		pm.pb_dac = PatchMatrix.new(
			server:s, target:gr.pb, action:\addAfter,
			in: bus_pb_idx,
			out: bus.dac.collect({ |b| b.index }),
			feedback:true
		);

		// playback -> record
		postln("softcut: pb->rec patchmatrix");
		pm.pb_rec = PatchMatrix.new(
			server:s, target:gr.pb, action:\addAfter,
			in: bus_pb_idx,
			out: bus_rec_idx,
			feedback:true
		);

		this.addCommands;

		nvoices.do({ arg i;
			this.addPoll(("phase_" ++ (i+1)).asSymbol, {
				var val = voices[i].phase_b.getSynchronous;
				postln("phase: " ++ val);
				val
			});
			this.addPoll(("phase_norm_" ++ (i+1)).asSymbol, {
				voices[i].phase_b.getSynchronous / voices[i].buf.duration
			});
		});


		s.sync;
	}

	addCommands {


		var com = [

			//-- voice functions
			[\start, \i, {|msg| msg.postln; voices.postln; voices[msg[1]-1].start; }],
			[\stop, \i, {|msg| voices[msg[1]-1].stop; }],
			[\reset, \i, {|msg| voices[msg[1]-1].reset; }],

			//-- direct control of synth params
			[\amp, \if, { |msg| voices[msg[1]-1].syn.set(\amp, msg[2]); }],
			[\rec, \if, { |msg| voices[msg[1]-1].syn.set(\rec, msg[2]); }],
			[\pre, \if, { |msg| voices[msg[1]-1].syn.set(\pre, msg[2]); }],
			[\rate, \if, { |msg| voices[msg[1]-1].syn.set(\rate, msg[2]); }],
			[\ratelag, \if, { |msg| voices[msg[1]-1].syn.set(\ratelag, msg[2]); }],
			[\loopStart, \if, { |msg| voices[msg[1]-1].syn.set(\start, msg[2]); }],
			[\loopEnd, \if, { |msg| voices[msg[1]-1].syn.set(\end, msg[2]); }],
			[\fade, \if, { |msg| voices[msg[1]-1].syn.set(\fade, msg[2]); }],
			[\loopFlag, \if, { |msg| voices[msg[1]-1].syn.set(\loop, msg[2]); }],
			[\fadeRec, \if, { |msg| voices[msg[1]-1].syn.set(\fadeRec, msg[2]); }],
			[\fadePre, \if, { |msg| voices[msg[1]-1].syn.set(\fadePre, msg[2]); }],
			[\recRun, \if, { |msg| voices[msg[1]-1].syn.set(\recRun, msg[2]); }],
			[\offset, \if, { |msg| voices[msg[1]-1].syn.set(\offset, msg[2]); }],
			[\preLag, \if, { |msg| voices[msg[1]-1].syn.set(\preLag, msg[2]); }],
			[\recLag, \if, { |msg| voices[msg[1]-1].syn.set(\recLag, msg[2]); }],
			[\envTimeScale, \if, { |msg| voices[msg[1]-1].syn.set(\envTimeScale, msg[2]); }],


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
