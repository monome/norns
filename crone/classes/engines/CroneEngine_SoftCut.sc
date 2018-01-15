// a sample capture / playback matrix
CroneEngine_SoftCut : CroneEngine {
	classvar nvoices = 8;
	//	classvar nbufs = 8;
	classvar bufdur = 64.0;

	classvar commands;

	var <s; // server
	var <bus; // busses
	var <buf; // buffers
	var <syn; // synths
	var <gr; // groups
	var <pm; // patch matrix
	var <rec; // recorders

	var <voices; // array of voices

	*initClass {
		StartUp {
			CroneDefs.add(	// triggered recording from arbitrary position
				SynthDef(\softcut_rec_trig_gate, {
					arg buf, in, gate=0,
					rate=1, start=0.0, end=1.0, loop=0,
					rec=1, pre=0, fade=0.01;

					var sr, brs,
					sin, sin_phase,
					phase, wr, trig,
					env_pre, env_rec;

					sr = SampleRate.ir;
					brs = BufRateScale.kr(buf); // NB: BfWr and BufWrPre are non-interpolating...
					env_rec = EnvGen.ar(Env.asr(fade, 1, fade), gate) * rec;
					env_pre = (pre * env_rec).max(1-env_rec);

					sin = In.ar(in);
					// FIXME: use Sweep or something, trigger bus to reset / start
					phase = Phasor.ar(gate, rate * brs, start*sr, end*sr, start);
					wr = BufWrPre.ar(sin * env_rec, buf, phase, env_pre);
				})
			);
		}
	}

	*new { arg server, group, in, out;
		^super.new.init(server, group, in, out).initSub(server, group, in, out);
	}

	kill {
		buf.do({ |b| b.free; });
		bus.rec.do({ |b| b.free; });
		syn.do({ arg synth; synth.free; });
	}

	//---  buffer and routing methods
	startRec { |id, pos| /* TODO */} // one-shot record
	stopRec { |id| /* TODO */}


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

	initSub {
		var com;
		var bus_pb_idx; // tmp collection of playback bus indices
		var bus_rec_idx;
		var bufcon;

		Routine {
			var s = server;

			postln("SoftCut: init routine");

			//--- groups
			gr = Event.new;
			gr.pb = Group.new(Crone.ctx.xg);
			gr.rec = Group.after(Crone.ctx.ig);
			// phase bus per voice (output)
			bus = Event.new;

			s.sync;

			// use an array of Conditions to delay execution until all buffers are allocated
			//			bufcon = Array.fill(nvoices, { Condition.new });

			postln("SoftCut: allocating buffers");
			//--- buffers
			buf = Array.fill(nvoices, { arg i;
				Buffer.alloc(s, s.sampleRate * bufdur, completionMessage: {
					//					bufcon[i].unhang;
				})
			});
			s.sync;
			//			bufcon.do({ arg con; con.hang; });

			postln("SoftCut: done waiting on buffer allocation");


			//-- voices
			voices = Array.fill(nvoices, { |i|
				SoftCutVoice.new(s, buf[i], s);
			});

			//-- 1-shot recorders; one per voice
			// TODO
			/*
			rec = buf.collect( {|b| Synth.newPaused(\softcut_rec_trig_gate, [
				\buf, b.bufnum, \in, bus.adc[0].index
			], gr.rec); });
			*/

			//--- patch matrices
			bus_pb_idx = voices.collect({ |v| v.bus.pb.index });
			bus_rec_idx = voices.collect({ |v| v.bus.rec.index });
			pm = Event.new;

			postln("softcut: in->rec patchmatrix");
			// input -> record
			pm.adc_rec = PatchMatrix.new(
				server:s, target:gr.rec, action:\addToTail,
				in: bus.adc.collect({ |b| b.index }),
				out: bus_rec_idx
			);
			postln("softcut: pb->out patchmatrix");
			// playback -> output
			pm.pb_dac = PatchMatrix.new(
				server:s, target:gr.pb, action:\addAfter,
				in: bus_pb_idx,
				out: bus.dac.collect({ |b| b.index })
			);

			// playback -> record
			postln("softcut: pb->rec patchmatrix");
			pm.pb_rec = PatchMatrix.new(
				server:s, target:gr.pb, action:\addAfter,
				in: bus_pb_idx,
				out: bus_rec_idx
			);

		}.play;

	} // initSub

	addCommands {

		var com = [
			// set output level of a playback voice
			[\level, \if, { |msg| voices[msg[1]-1].level_(msg[2]) }],
			// set mute flag for playback voice
			[\mute, \if, { |msg| }],//pb[msg[1]-1].mute_(msg[2]) }],
			// cut playback to position
			[\pos, \if, { |msg| }], //pb[msg[1]-1].pos_(msg[2]) }],
			// set playback to new rate, with crossfade
			[\rate, \if, { |msg| }], //pb[msg[1]-1].rate_(msg[2]) }],
			// set crossfade time for given playback voice
			[\fade, \if, { |msg| }], //pb[msg[1]-1].fade_(msg[2]) }],
			// set crossfade curve for given playback voice
			[\curve, \if, { |msg| }], //pb[msg[1]-1].curve_(msg[2]) }],
			// set source buffer for playback voice
			[\buf, \ii, { |msg| }], //pb[msg[1]-1].buffer_(buf[msg[2]-1]) }],
			// set (raw) looping behavior for playback voice.
			[\loop, \ii, { |msg| }], //pb[msg[1]-1].loop_(msg[2]) }],
			//-- control recording
			// start recording given buffer at last set position
			[\rec, \i, { |msg| }], //rec[msg[1]-1].start }],
			// start recording given buffer at given position
			[\rec_pos, \if, { |msg| }], //rec[msg[1]-1].start(msg[2]) }],
			// stop recording given buffer
			[\rec_stop, \i, { |msg| }], //rec[msg[1]-1].stop }],
			// set record level for given buffer
			[\rec_level, \if, { |msg| }], //rec[msg[1]-1].rec_(msg[2]) }],
			// set prerecord (overdub) level for given buffer
			[\rec_pre, \if, { |msg| }], //rec[msg[1]-1].pre_(msg[2]) }],
			// set loop flag for given recorder.
			[\rec_loop, \ii, { |msg| }], //rec[msg[1]-1].loop_(msg[2]) }],
			//-- routing
			// level from given ADC channel to given recorder
			[\adc_rec, \iif, { |msg| }], //pm.adc_rec(msg[1]-1, msg[2], msg[3]); }],
			// level from given playback channel to given recorder
			[\play_rec, \iif, { |msg| }], //pm.pb_rec.level_(msg[1]-1, msg[2], msg[3]); }],
			// level from given playback channel to given DAC channel
			[\play_dac, \iif, { |msg| }], //pm.pb_dac.level_(msg[1]-1, msg[2], msg[3]); }],
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

}
