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
	var gr; // groups
	var <pb; // playback voice
	var <pm; // patch matrix
	var <rec; // recorders

	*initClass {
		CroneDefs.add( // looped, crossfaded playback + recording3
			SynthDef.new(\softcuthead, {
				arg buf, in, out, phase_out= -1, trig_out= -1, trig_in = -1,
				amp=0.2, rec=0.0, pre=0.0,
				rate=1, ratelag = 0.1,
				start=0, end=1, fade=0.1, loop=1,
				fadeRec=1.0, fadePre = 1.0, recRun = 0, recOff=0;

				var snd, phase, tr;
				var brs;
				var softcut;
				var trig;
				var sin;

				brs = BufRateScale.kr(buf);
				trig = InTrig.kr(trig_in);
				sin = In.ar(in);
				pre = Lag.ar(K2A.ar(pre), 0.1);
				rec = Lag.ar(K2A.ar(rec), 0.1);
				rate = Lag.ar(K2A.ar(rate), ratelag);
				
				softcut =  SoftCutHead.ar(buf, sin, trig,
					rate * brs, start, end, fade, loop,
					rec, pre, fadeRec, fadePre, recRun, recOff);
				
				phase = softcut[0];
				tr = softcut[1];
				snd = softcut[2];
				
				Out.ar(out, (snd*amp).dup);
				Out.ar(phase_out, phase);
				Out.ar(trig_out, tr);
			})
		);

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
	
	*new { arg server, group, in, out;
		^super.new.init(server, group, in, out).initSub(server, group, in, out);
	}

	kill {
		buf.do({ |b| b.free; });
		bus.rec.do({ |b| b.free; });
        syn.do({ arg synth; synth.free; });
    }

    // set voice parameters
    voiceLevel { |id, level| syn.voice[id].set(\amp, level); }
    voicePos { |id, pos| bus.rd_phase.set(1); }
    // trigger jump to start
    voiceTrigger { |id| bus.loop_trig[id].set(1); }
    voiceStart { |id, val| }
    voiceStop { }
    voiceOffset { }
    voiceRecLevel { |id, level| }
    voicePreLevel { |id, level| }			
    voiceRecFade { |id, val| }
    voiceRate { |id,val| }
    voiceLoopStart { |id,val| }
    voiceLoopEnd { |id,val| }
    voiceLoopFlag { |id, val| syn.voice[id].set(\loop, val); }
    
    // start/stop one-shot recording
    startRec { |id, pos| }
    stopRec { |id| }
    // manage buffers
    readBuf { |id, path| }
    writeBuf { |id, path| }
    trimBuf { |id, start, end| }
    normBuf { |id, level| }
    // set routing
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

            // FIXME: do wwe really need these extra groups?
            // groups: adc -> playback -> record -> dac
            postln("SoftCut: adc group");
            gr.adc = Group.new(group); s.sync;
            postln("SoftCut: playback group");
            gr.pb = Group.after(gr.adc); s.sync;
            postln("SoftCut: record group");
            gr.rec = Group.after(gr.pb); s.sync;
            postln("SoftCut: dac group"); 
            gr.dac = Group.after(gr.rec); s.sync;

            // phase bus per voice (output)
			bus = Event.new;
            bus.rd_phase = Array.fill(nvoices, { Bus.audio(s, 1) });
            // loop-trigger bus per voice (output)
            bus.loop_trig = Array.fill(nvoices, { Bus.audio(s, 1) });
            // control-rate copies for polling
            bus.rd_phase_kr = Array.fill(nvoices, { Bus.control(s, 1) });
            bus.loop_trig_kr = Array.fill(nvoices, { Bus.control(s, 1) });
            
            s.sync;
            
            bus_pb_idx = bus.pb.collect({ |b| b.index });
            bus_rec_idx = bus.rec.collect({ |b| b.index });
            
            // use an array of Conditions to delay execution until all buffers are allocated
            bufcon = Array.fill(nvoices, { Condition.new });
            //--- buffers
            buf = Array.fill(nvoices, { arg i;
                Buffer.alloc(s, s.sampleRate * bufdur, completionMessage: {
                    bufcon[i].unhang;
                })				
            });

            bufcon.do({ arg con; con.hang; });

            postln("SoftCut: done waiting on buffer allocation");
            
            //--- patch matrices
            pm = Event.new;
            
            // input -> record
            pm.adc_rec = PatchMatrix.new(
                server:s, target:gr.adc, action:\addAfter,
                in: bus.adc.collect({ |b| b.index }),
                out: bus.rec.collect({ |b| b.index })
            );		
            // playback -> output
            pm.pb_dac = PatchMatrix.new(
                server:s, target:gr.pb, action:\addAfter,
                in: bus_pb_idx,
                out: bus.dac.collect({ |b| b.index })
            );
            
            // playback -> record
            pm.pb_rec = PatchMatrix.new(
                server:s, target:gr.pb, action:\addAfter,
                in: bus_pb_idx,
                out: bus_pb_idx
            );

            //--- IO synths
            syn = Event.new;

            // arrary of synths for synchronized record/playback
            syn.voice = Array.fill(nvoices, { |i|
                Synth.new(\softcutvoice, [
                    \buf, buf[i].bufnum,
                    \in, bus.rec[i].index,
                    \out, bus.pb[i].index
                ], );
            });

        }.play;


}



}
