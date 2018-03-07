// TODO: samples currently overlap which is evident when using long samples and/or high tempo - better to cut out 
Engine_Step : CroneEngine {
    classvar maxNumPatternSteps = 128;
    classvar maxNumSamples = 32;
	var group;
    var tempoSpec;
    var swingAmountSpec;
    var synthDefName = 'step';
    var numSamples = 8;
    var numPatternSteps;
    var patternBeats;

    var buffers, trigs;
    var player, <playpos;
    var tempo, swingAmount;
    var currentSwingOffset;
    var clock;

	*defaultServerLatency {
		^0.03 // TODO: for swift led response. is this too low? check if this causes late events on the norns.
	}

	*new { |context, callback| ^super.new(context, callback).initStep }

	initStep {
    	tempoSpec = ControlSpec(20, 999, step: 1, default: 120);
    	swingAmountSpec = ControlSpec(0, 100, step: 1, default: 0);
        clock = TempoClock.new;
	}

	alloc {
		group = Group.new(context.xg);

        buffers = maxNumSamples.collect { Buffer.new };

	    trigs = Array.fill2D(maxNumSamples, maxNumPatternSteps) { \rest };

	    SynthDef('step', { |out=0, bufnum|
	        OffsetOut.ar( out, Pan2.ar(PlayBuf.ar(1, bufnum, BufRateScale.kr(bufnum), doneAction: 2)) );
	    }).add;

	    SynthDef('stepOut', { |out=0, bufnum|
	        Out.ar( out, Pan2.ar(PlayBuf.ar(1, bufnum, BufRateScale.kr(bufnum), doneAction: 2)) );
	    }).add;

		context.server.sync;

		this.scrambleSamples; // TODO: temporary, instead aim for seamless persistence, handled in lua script

		this.setDefaults;

		this.addCommand(\scrambleSamples, "") { |msg|
            fork {
			    this.scrambleSamples;
            };
		};

		this.addCommand(\loadSample, "if") { |msg| this.loadSample(msg[1], msg[2]) };
		this.addCommand(\clearAllTrigs, "") { this.clearAllTrigs };
		this.addCommand(\setTrig, "ii") { |msg| this.setTrig(msg[1], msg[2]) };
		this.addCommand(\clearTrig, "ii") { |msg| this.clearTrig(msg[1], msg[2]) };
		this.addCommand(\playSequencer, "") { this.play };
		this.addCommand(\stopSequencer, "") { this.stop };
		this.addCommand(\setSwingAmount, "i") { |msg| this.setSwingAmount(msg[1]) }; // swing amount > 0 will delay odd steps
		this.addCommand(\setTempo, "i") { |msg| this.setTempo(msg[1]) };

		// TODO: playpos not working right now
 		// TODO: playpos change is currently quantized to 2 steps (fix this)
		this.addCommand(\playpos, "i") { |msg| this.setPlaypos(msg[1]) };

		this.addCommand(\setNumSteps, "i") { |msg| this.setNumPatternSteps(msg[1]) }; // will stop sequencer
		this.addCommand(\setNumSamples, "i") { |msg| this.setNumSamples(msg[1]) }; // will stop sequencer

/*
		TODO: future idea: NRT rendering to soundfile is supported out of the box with SuperCollider Patterns using

			Pattern.record(path, headerFormat: "AIFF", sampleFormat: "float", numChannels: 2, dur, fadeTime: 0.2, clock, protoEvent, server, out: 0, outNumChannels)

		... calculating dur argument automatically based on tempo & pattern length

		this.addCommand(\renderPattern, "i") { |msg|
			this.spawnPattern(repeats, true).deepCopy;
		};
*/

		this.addCommand(\useOffsetOut, "i") { |msg| // TODO: temporary, to remove
			var wasPlaying = player.isPlaying;
	        player.isPlaying.if { this.stop };

			if (msg[1].asBoolean) {
				synthDefName = 'step';
			} {
				synthDefName = 'stepOut';
			};

			wasPlaying.if { this.play };
		};

		this.addCommand(\useSchedLatency, "i") { |msg| // TODO: temporary, to remove
			if (msg[1].asBoolean) {
				context.server.latency = this.class.defaultServerLatency;
			} {
				context.server.latency = nil;
			}
		};

		this.addPoll('playpos') { playpos ? -1 }; // TODO: weirdness
	}

	free {
        this.stop;
		group.free;
        buffers do: _.free;
		super.free;
	}

	setDefaults {
		// this.setNumSamples(8);  // TODO: (better to have here than in declaration)
        this.setNumPatternSteps(16);
        this.setTempo(tempoSpec.default);
        this.setSwingAmount(swingAmountSpec.default);
	}

	scrambleSamples {
		var filenameSymbol = this.class.filenameSymbol;
		var soundsFolder = PathName(filenameSymbol.asString).pathOnly ++ "tests";
		var soundsToLoad;
		var allSounds = PathName(soundsFolder)
			.deepFiles
			.select { |pathname| ["aif", "aiff", "wav"].includesEqual(pathname.extension) }
			.collect(_.fullPath);

		soundsToLoad = allSounds
			.scramble
			.keep(numSamples);

		soundsToLoad.do { |path, samplenum|
			this.loadSample(samplenum, path.asString);
		};

		context.server.sync;

		"% randomly selected sounds out of % sounds in folder % loaded into step"
				.format(soundsToLoad.size, allSounds.size, soundsFolder.quote).inform;
	}

	loadSample { |samplenum, path|
		if (samplenum >= 0 and: samplenum < numSamples) {
			// TODO: support stereo samples
			// TODO: enforce a maximum number of frames - no need to load 1 minute soundfiles (informative warning when not whole file is loaded)
	        buffers[samplenum].allocReadChannel(path, channels: [0], completionMessage: {
	            this.changed(\sampleWasLoaded, samplenum, path);
	        }); // TODO: look into making a PR for completionMessage bug
		} {
			"Invalid argument (%) to loadSample. Sample number must be between 0 and %"
				.format(samplenum, maxNumSamples-1).error;
		};
	}

	setNumSamples { |argNumSamples|
		if ((argNumSamples > 0) and: (argNumSamples <= maxNumSamples)) {
			var wasPlaying = player.isPlaying;
	        player.isPlaying.if { this.stop };
			this.clearAllTrigs;
			fork {
				if (argNumSamples < numSamples) {
					(argNumSamples..maxNumSamples).do { |samplenum|
						buffers[samplenum].zero;
					};
				};
				context.server.sync;
				wasPlaying.if { this.play };
				numSamples = argNumSamples;
			};
		} {
			"Invalid argument (%) to setNumSamples. Number of samples must be between 1 and %"
				.format(argNumSamples, maxNumSamples).error;
		};
	}

	setNumPatternSteps { |argNumPatternSteps|
		if ((argNumPatternSteps > 0) and: (argNumPatternSteps <= maxNumPatternSteps)) {
			var wasPlaying = player.isPlaying;
	        player.isPlaying.if { this.stop };
			numPatternSteps = argNumPatternSteps;
			patternBeats = argNumPatternSteps div: 4; // TODO: make patternBeats customizable?
			wasPlaying.if { this.play };
		} {
			"Invalid argument (%) to setNumPatternSteps. Number of pattern steps must be between 1 and %"
				.format(argNumPatternSteps, maxNumPatternSteps).error;
		};
	}

/*
 	TODO: make patternBeats customizable?
	setPatternBeats { |beats|
		// FIXME: validate beats
		this.stop;
		patternBeats = beats;
		this.play;
	}
*/

    setTempo { |bpm|
        tempo = tempoSpec.constrain(bpm);
        clock.tempo_(tempo/60);
    }

    setPlaypos { |argPlaypos|
		if ((argPlaypos >= 0) and: (argPlaypos < maxNumPatternSteps)) {
			playpos = argPlaypos;
		} {
			"Invalid argument (%) to setPlaypos. playpos must be between 0 and %"
				.format(argPlaypos, maxNumPatternSteps-1).error;
		};
    }

    setSwingAmount { |amount|
        var maxSwingTimingOffset;
        maxSwingTimingOffset = (patternBeats/numPatternSteps)/2; // FIXME: something's not right here
        swingAmount = swingAmountSpec.constrain(amount);
        currentSwingOffset = maxSwingTimingOffset * swingAmount / 100;
    }

    clearAllTrigs {
        numSamples.do { |samplenum|
            maxNumPatternSteps.do { |stepnum|
                this.trigIsSet(samplenum, stepnum).if {
					this.clearTrig(samplenum, stepnum);
				};
            };
        };
    }

    setTrig { |samplenum, stepnum|
        trigs[samplenum][stepnum] = 1;
    }

    clearTrig { |samplenum, stepnum|
        trigs[samplenum][stepnum] = \rest;
    }

    trigIsSet { |samplenum, stepnum| ^trigs[samplenum][stepnum] == 1 }

    spawnPattern { |repeats, excludePlayposBumping=false|
        var sampleTriggering, timingAndSwing, playposBumping;

        timingAndSwing = Pbind(*[
            dur: Prout({
                var noSwingDur = patternBeats/numPatternSteps;
                (numPatternSteps/2) do: {
                    var swingOffset;
                    swingOffset = currentSwingOffset;
                    (noSwingDur+swingOffset).yield;
                    (noSwingDur-swingOffset).yield;
                };
                nil.yield;
            });
        ]);

        excludePlayposBumping.not.if {
            playposBumping = Pbind(*[
                note: \rest,
/*
                tickFunc: Pfunc({
                    playpos = 0;
					if (playpos < numPatternSteps) {
						var oldPlaypos = playpos;
						playpos = playpos + 1;
					}
                })
*/
                tickFunc: Prout({
                    playpos = 0;
                    loop {
                        if (playpos < numPatternSteps) { playpos } { nil }.yield;
						playpos = playpos + 1;
                    }
                })
            ]);
        };

        sampleTriggering = buffers.collect { |buffer, i|
            Pbind(*[
				out: context.out_b,
				group: group,
                instrument: synthDefName,
                bufnum: buffer, // TODO: only play buffer if it has been fully loaded, to remedy "BOOOM" issues
                note: Pseq(trigs[i], 1) // TODO: to only play buffer if it has been loaded can probably be remidied by adjusting the trigs[i] array accordingly
				// TODO: "and" the Pseq with sampleIsLoaded predicate
            ])
        };

        ^Ppar(
            (sampleTriggering ++ playposBumping).collect { |pattern| Pchain(pattern, timingAndSwing) },
            repeats
        );
    }

	sampleIsLoaded { |samplenum| // TODO: handle sample names in an array separate from buffers
		^buffers[samplenum].path.notNil
	}

    play { |repeats=inf|
        player.isPlaying.if {
            "Already playing...".inform;
        } {
			// TODO: change so that pattern is always spawned
            player = if (player.isNil) {
                "Playing...".inform;
                this.spawnPattern(repeats).asEventStreamPlayer;
            } {
                "Resuming pattern".inform;
                player;
            };
            player.play(clock);
        };
    }

    stop {
        player.isPlaying.if {
            player.stop;
			player = nil; // TODO: make this prettier
            playpos = nil;
            "...Stopped".inform;
        };
    }

/*
	TODO: optionally send this kind of content via /poll/data
	refreshGrid {
		numSamples.collect { |samplenum|
		    numPatternSteps.do.collect { |stepnum|
		        if (this.trigIsSet(samplenum, stepnum) or: (playpos == stepnum)) { 1 << stepnum } { 0 };
		    }.sum;
		}.debug;
	}
*/
}
