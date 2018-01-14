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

	var <voice; // array of voices
	
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
	startRec { |id, pos| } // one-shot record
	stopRec { |id| }
	readBuf { |id, path| }
	writeBuf { |id, path| }
	trimBuf { |id, start, end| }
	normBuf { |id, level| }
	playRecLevel { |srcId, dstId, level| }
	adcRecLevel { |srcId, dstId, level| }

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
			gr.rec = Group.new(Crone.ctx.ig);
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
			voice = Array.fill(nvoices, { |i|
				SoftCutVoice.new(s, buf[i], s);
			});
			
			bus_pb_idx = voice.collect({ |v| v.bus.pb.index });
			bus_rec_idx = voice.collect({ |v| v.bus.rec.index });
		
			
			//--- patch matrices
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


	}

}
