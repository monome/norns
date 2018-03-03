Engine_JustSample : CroneEngine {
	classvar maxSampleLength = 8;
	classvar delayTimeSpec;
	classvar decayTimeSpec;
	classvar rqSpec;
	classvar cutoffSpec;
	classvar speedSpec;
	var buffer;
	var sampleLength;
	var recSynth;
	var playSynth;
	var serverLatency;
	var stopRecordingRoutine;
	var startedRecording;

	*initClass {
		// delayTimeSpec = \delay.asSpec;
		delayTimeSpec = ControlSpec(0.0001, 3, 'exp', 0, 0.3, " secs");
		// decayTimeSpec = \delay.asSpec;
		decayTimeSpec = ControlSpec(0.0001, 3, 'exp', 0, 0.3, " secs");
		rqSpec = \rq.asSpec;
		// cutoffSpec = \freq.asSpec;
        cutoffSpec = ControlSpec(20, 10000, 'exp', 0, 440, " Hz");
        speedSpec = ControlSpec(0.125, 8, 'exp', 0, 1, "");
	}

	alloc {
		var monoRecSynthDef;
		var stereoRecSynthDef;
		var playSynthDef;

		monoRecSynthDef = SynthDef(\monoRecord, {
            arg
                in,
                bufnum,
                gate=1,
                monitor
            ;
			// var fade = 0.01; // TODO: move fade env to play
			var env;
			var sig;
			// env = EnvGen.ar(Env.asr(fade, 1, fade), gate, doneAction: Done.freeSelf);
			EnvGen.ar(Env.cutoff(0.01), gate, doneAction: Done.freeSelf);
			sig = In.ar(in) ! 2;
			// sig = sig * env;
			RecordBuf.ar(sig, bufnum, loop: 0);
            Out.ar(monitor, sig);
		}).add;

		stereoRecSynthDef = SynthDef(\stereoRecord, {
            arg
                in, // TODO: assume adjacent in busses and SynthDef.wrap
                bufnum,
                gate=1,
                monitor
            ;
			// var fade = 0.01; // TODO: move fade env to play
			var env;
			var sig;
			// env = EnvGen.ar(Env.asr(fade, 1, fade), gate, doneAction: Done.freeSelf);
			EnvGen.ar(Env.cutoff(0.01), gate, doneAction: Done.freeSelf);
			sig = In.ar(in, 2); // TODO: why are inbusses 2xmono ?
			RecordBuf.ar(sig, bufnum, loop: 0);
            Out.ar(monitor, sig);
		}).add;

/*
    TODO
		var makeRecSynthDef = { |defName, numChannels|
			SynthDef(defName, {
	            arg
	                in,
	                bufnum,
	                gate=1,
	                monitor
	            ;
				// var fade = 0.01; // TODO: move fade env to play
				var env;
				var sig;
				// env = EnvGen.ar(Env.asr(fade, 1, fade), gate, doneAction: Done.freeSelf);
				EnvGen.ar(Env.cutoff(0.01), gate, doneAction: Done.freeSelf);
				sig = In.ar(in, numChannels) ! (numChannels-1); // TODO: why are inbusses 2xmono ?
				// sig = sig * env;
				RecordBuf.ar(sig, bufnum, loop: 0, doneAction: Done.freeSelf);
	            Out.ar(monitor, sig);
			});
		};

		makeRecSynthDef.value(\monoRecord, 1).add;
		makeRecSynthDef.value(\stereoRecord, 2).add;
*/

		playSynthDef = SynthDef(\play, {
			arg
				out,
				bufnum,
				numFrames,
				gate=1,
				phasorFreq=1,
				startPos,
				endPos,
				delayLevel,
				delayTime,
				decayTime,
				cutoffFreq,
				rq,
				reverbLevel,
				reverbRoom,
				reverbDamp
			;
			var env;
			var playhead;
			var sig;

			env = EnvGen.ar(Env.cutoff(0.01), gate, doneAction: Done.freeSelf);
			playhead = Phasor.ar(0, BufRateScale.kr(bufnum) * phasorFreq * sign(startPos-endPos) * (-1), numFrames*startPos, numFrames*endPos);
			// TODO: add miller puckette-style amp window to remove clicks
			sig = BufRd.ar(2, bufnum, playhead, interpolation: 4);

			sig = RLPF.ar(sig, cutoffFreq, rq);
			sig = sig + (delayLevel * CombC.ar(sig, maxdelaytime: delayTimeSpec.maxval, delaytime: delayTime, decaytime: decayTime));
			sig = sig + (reverbLevel * FreeVerb2.ar(sig, sig, 1.0, reverbRoom, reverbDamp));
			Out.ar(out, sig);
		},
            [
                nil, // out
                nil, // bufnum
                nil, // numFrames
                nil, // gate
                0.02, // phasorFreq
                0.1, // startPos
                0.1, // endPos
                0.02, // delayLevel
                0.25, // delayTime
                0.02, // decayTime
                0.02, // cutoffFreq
                0.02, // rq
                0.02, // reverbLevel
                0.02, // reverbRoom
                0.02 // reverbDamp
            ]).add;

		buffer = Buffer.alloc(numChannels: 2, numFrames: this.maxBufferNumFrames);

		context.server.sync;

		this.addCommand("record", "") { |msg| this.record };
		this.addCommand("play", "") { |msg| this.play };
		this.addCommand("startPos", "f") { |msg| this.startPos(msg[1]) };
		this.addCommand("endPos", "f") { |msg| this.endPos(msg[1]) };
		this.addCommand("speed", "f") { |msg| this.speed(msg[1]) };
		this.addCommand("cutoff", "f") { |msg| this.cutoff(msg[1]) };
		this.addCommand("resonance", "f") { |msg| this.resonance(msg[1]) };
		this.addCommand("delayLevel", "f") { |msg| this.delayLevel(msg[1]) };
		this.addCommand("delayTime", "f") { |msg| this.delayTime(msg[1]) };
		this.addCommand("decayTime", "f") { |msg| this.decayTime(msg[1]) };
		this.addCommand("reverbLevel", "f") { |msg| this.reverbLevel(msg[1]) };
		this.addCommand("reverbRoom", "f") { |msg| this.reverbRoom(msg[1]) };
		this.addCommand("reverbDamp", "f") { |msg| this.reverbDamp(msg[1]) };
		this.addCommand("readBuffer", "s") { |msg| this.readBuffer(msg[1]) };
		this.addCommand("writeBuffer", "s") { |msg| this.writeBuffer(msg[1]) };
	}

	free {
		buffer.free;
		recSynth.release;
		playSynth.release;
		// TODO: probably d_free synthdefs too to really clean up and not cause clashes
	}

	record {
		startedRecording = SystemClock.seconds;
		context.server.makeBundle(serverLatency) {
			recSynth !? _.release;
			playSynth !? {
                playSynth.release;
			    playSynth = nil;
            };
			recSynth = Synth(
                // \monoRecord, // TODO: stereoRecord
                \stereoRecord,
                args: [
                    \in, context.in_b,
                    \bufnum, buffer,
                    \monitor, context.out_b
                ],
                target: context.xg,
                addAction: \addToTail
            );
		};
		postln("started recording");
		stopRecordingRoutine = fork {
			maxSampleLength.wait;
            maxSampleLength.debug(\sc_maxSampleTimeReached);
            this.play;
		};
	}

	play {
		if (startedRecording.notNil) {
			sampleLength = SystemClock.seconds - startedRecording;
			postln("seconds sampled:" + sampleLength);
            stopRecordingRoutine.stop;
		};
		context.server.makeBundle(serverLatency) {
			recSynth !? {
                recSynth.release;
                recSynth = nil;
            };
			playSynth !? _.release;
			playSynth = Synth(
                \play,
                args: [
                    \out, context.out_b,
                    \bufnum, buffer,
                    \numFrames, this.actualBufferNumFrames,

				    \startPos, 0,
				    \endPos, 1,
				    \phasorFreq, 1, // speed
                    // pitch
				    \cutoffFreq, cutoffSpec.maxval,
				    \rq, rqSpec.maxval,
				    \delayLevel, 0,
				    \reverbLevel, 0,
				    \delayTime, delayTimeSpec.default,
				    \decayTime, decayTimeSpec.default,
				    \reverbRoom, 0.5,
				    \reverbDamp, 0.5
                ],
                target: context.xg,
                addAction: \addToHead
            );
		};
	}

	actualBufferNumFrames {
		^(sampleLength*context.server.sampleRate).ceil
	}

	maxBufferNumFrames {
		^(maxSampleLength*context.server.sampleRate).ceil
	}

	writeBuffer { |path| buffer.write(path) }

	readBuffer { |path| buffer.read(path, numFrames: this.actualBufFrames) }

	startPos { |value| // 0..1.0
		playSynth !? {
            playSynth.set(
                \startPos,
                value.debug(\sc_startPos_in).clip(0, 1).debug(\sc_startPos_clipped)
            )
        };
	}

	endPos { |value| // 0..1.0
		playSynth !? {
            playSynth.set(
                \endPos,
                value.debug(\sc_endPos_in).clip(0, 1).debug(\sc_endPos_clipped)
            )
        };
	}

	speed { |value| // 0..1
		playSynth !? {
            playSynth.set(
                \phasorFreq,
                speedSpec.map(value.debug(\sc_speed_in)).debug(\sc_speed_mapped)
            )
        };
	}

	cutoff { |value| // 0..1.0
		playSynth !? {
            playSynth.set(
                \cutoffFreq,
                cutoffSpec.map(value.debug(\sc_cutoff_in)).debug(\sc_cutoff_mapped)
            )
        };
	}

	resonance { |value| // 0..1.0
		playSynth !? {
            playSynth.set(
                \rq,
                rqSpec.map((value.debug(\sc_resonance_in).neg+1).debug(\_sc_resonance_neg_plus_1)).debug(\sc_resonance_constrained)
            )
        };
	}

	delayLevel { |value| // 0..1.0
		playSynth !? {
			playSynth.set(
				\delayLevel,
				value.debug(\sc_delayLevel_in).clip(0, 1).debug(\sc_delayLevel_clipped)
			)
		};
	}

	reverbLevel { |value| // 0..1.0
		playSynth !? {
			playSynth.set(
				\reverbLevel,
				value.debug(\sc_reverbLevel_in).clip(0, 1).debug(\sc_reverbLevel_clipped)
			)
		};
	}

	delayTime { |value| // \delay.asSpec
		playSynth !? {
			playSynth.set(
				\delayTime,
				delayTimeSpec.map(value.debug(\sc_delayTime_in)).debug(\sc_delayTime_map)
			)
		};
	}

	decayTime { |value|
		playSynth !? {
			playSynth.set(
				\decayTime,
				decayTimeSpec.map(value.debug(\sc_decayTime_in)).debug(\sc_decayTime_map)
			)
		};
	}

	reverbRoom { |value| // 0..1
		playSynth !? {
			playSynth.set(
				\reverbRoom,
				value.debug(\sc_reverbRoom_in).clip(0, 1).debug(\sc_reverbRoom_clipped)
			)
		};
	}

	reverbDamp { |value| // 0..1
		playSynth !? {
            playSynth.set(
                \reverbDamp,
                value.debug(\sc_reverbDamp_in).clip(0, 1).debug(\sc_reverbDamp_clipped)
            )
        };
	}

}
