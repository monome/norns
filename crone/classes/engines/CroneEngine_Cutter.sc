// a sample capture / playback matrix
CroneEngine_Cutter : CroneEngine {
	classvar nvoices = 4;
	classvar nbufs = 4;
	classvar bufdur = 64.0;
	
	var <s; // server
	var <gr; // groups
	var <bus; // busses
	var <buf; // buffers
	var <syn; // synths
	var <pb; // playback voice
	var <pm; // patch matrix
	var <rec; // recorders
	
	*new { arg server, group, in, out;
		^super.new.init(server, group, in, out).initSub();
	}

	kill {
		buf.do({ |b| b.free; });
		bus.rec.do({ |b| b.free; });
		^super.kill;
	}
	
	initSub {
		var com;
		var pb_out_b;
		var bufcon;

		Routine {
			var s = server;
			
			postln("Cutter: init routine");

			//--- groups
			gr = Event.new;
			// groups: adc -> playback -> record -> dac
			postln("Cutter: adc group");
			gr.adc = Group.new(group); s.sync;
			postln("Cutter: playback group");
			gr.pb = Group.after(gr.adc); s.sync;
			postln("Cutter: record group");
			gr.rec = Group.after(gr.pb); s.sync;
			postln("Cutter: dac group"); 
			gr.dac = Group.after(gr.rec); s.sync;

			s.sync;
			
			//--- busses
			bus = Event.new;
			// use the busses supplied by audio context
			bus.adc = Array.fill(2, { |i| Bus.new('audio', in_b[i].index, 1, s); }
			bus.dac = Array.fill(2, { |i| Bus.new('audio', in_b.index + i, 1, s) });			
			bus.rec = Array.fill(nbufs, { Bus.audio(s, 1) });

			s.sync;
			
			// use an array of Conditions to delay execution until all buffers are allocated
			bufcon = Array.fill(nbufs, { Condition.new });
			//--- buffers
			buf = Array.fill(nbufs, { arg i;
				Buffer.alloc(s, s.sampleRate * bufdur, completionMessage: {
					bufcon[i].unhang;
				})
				
			});

			bufcon.do({ arg con; con.hang; });

			postln("Cutter: done waiting on buffer allocation");
			
			//--- playback
			pb = Array.fill(nvoices, { arg i;
				CutFadeVoice.new(s, buf[i % nbufs], gr.pb)
			});

			//-- record
			rec = buf.collect({ |bf, i|
				SoftRecord.new(s, bf.bufnum, bus.rec[i].index)
			});

			//--- patch matrices
			pm = Event.new;
			pb_out_b = pb.collect({ |v| v.out_b.index });
			
			// input -> record
			pm.adc_rec = PatchMatrix.new(
				server:s, target:gr.adc, action:\addAfter,
				in: bus.adc.collect({ |b| b.index }),
				out: bus.rec.collect({ |b| b.index })
			);		
			// playback -> output
			pm.pb_dac = PatchMatrix.new(
				server:s, target:gr.pb, action:\addAfter,
				in: pb_out_b, 
				out: bus.dac.collect({ |b| b.index })
			);		
			// playback -> record
			pm.pb_rec = PatchMatrix.new(
				server:s, target:gr.pb, action:\addAfter,
				in: pb_out_b,
				out: pb_out_b
			);

			//--- IO synths
			syn = Event.new;
			syn.adc = Array.fill(2, { |i|
				Synth.new(\patch_mono, [
					\in, in_b[i].index, \out, bus.adc[i].index
				], gr.adc)
			});
			syn.dac = Array.fill(2, { |i|
				Synth.new(\patch_mono, [
					\in, bus.dac[i].index, \out, out_b.index + i
				], gr.dac)
			});
		}.play;
		
		// build commands list
		com = [
			//-- manipulate voice methods directly
			// set output level of a playback voice
			[\level, \if, { |msg| pb[msg[1]-1].level_(msg[2]) }],
			// set mute flag for playback voice
			[\mute, \if, { |msg| pb[msg[1]-1].mute_(msg[2]) }],
			// cut playback to position
			[\pos, \if, { |msg| pb[msg[1]-1].pos_(msg[2]) }],
			// set playback to new rate, with crossfade
			[\rate, \if, { |msg| pb[msg[1]-1].rate_(msg[2]) }],
			// set crossfade time for given playback voice
			[\fade, \if, { |msg| pb[msg[1]-1].fade_(msg[2]) }],
			// set crossfade curve for given playback voice
			[\curve, \if, { |msg| pb[msg[1]-1].curve_(msg[2]) }],
			// set source buffer for playback voice
			[\buf, \ii, { |msg| pb[msg[1]-1].buffer_(buf[msg[2]-1]) }],
			// set (raw) looping behavior for playback voice.
			[\loop, \ii, { |msg| pb[msg[1]-1].loop_(msg[2]) }],
			//-- control recording
			// start recording given buffer at last set position
			[\rec, \i, { |msg| rec[msg[1]-1].start }],
			// start recording given buffer at given position
			[\rec_pos, \if, { |msg| rec[msg[1]-1].start(msg[2]) }],
			// stop recording given buffer
			[\rec_stop, \i, { |msg| rec[msg[1]-1].stop }],
			// set record level for given buffer
			[\rec_level, \if, { |msg| rec[msg[1]-1].rec_(msg[2]) }],
			// set prerecord (overdub_ level for given buffer
			[\rec_pre, \if, { |msg| rec[msg[1]-1].pre_(msg[2]) }],
			// set loop flag for given recorder.
			[\rec_loop, \ii, { |msg| rec[msg[1]-1].loop_(msg[2]) }],
			//-- routing
			// level from given ADC channel to given recorder
			[\adc_rec, \iif, { |msg| pm.adc_rec(msg[1]-1, msg[2], msg[3]); }],
			// level from given playback channel to given recorder
			[\play_rec, \iif, { |msg| pm.pb_rec.level_(msg[1]-1, msg[2], msg[3]); }],
			// level from given playback channel to given DAC channel
			[\play_dac, \iif, { |msg| pm.pb_dac.level_(msg[1]-1, msg[2], msg[3]); }],
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
			[\norm, \if, { |msg| this.normalizeBuf(msg[1]-1, msg[2]) }],
			
		];

		com.do({ arg comarr;
			postln("adding command: " ++ comarr);
			this.addCommand(comarr[0], comarr[1], comarr[2]);
		});
	}

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
		startsamp = start * s.sampleRate;
		endsamp = end * s.sampleRate;
		samps = endsamp - startsamp;
		newbuf = Buffer.alloc(s, samps);
		buf[i].copyData(newbuf, 0, startsamp, samps);
		// any voices using this buffer need to be reassigned
		pb.do({ arg p;
			if(p.buf == buf[i], {
				p.buffer_(newbuf);
			});
			buf[i].free;
			buf[i] = newbuf;
		});
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

}
