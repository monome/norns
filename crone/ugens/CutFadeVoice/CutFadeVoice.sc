CutFadeVoice : MultiOutUGen {
	// output is 3+ channels: #phase_rd, trig, audio...
    *ar { arg bufnum=0, in=0, trigger=0, rate=1.0, start=0.0, end=1.0, fade=0.1, loop=1, rec=0, pre=0, fadeRec=0.0, fadePre=1.0, recRun=0, recOffset=0;
		^this.multiNew('audio', bufnum, in, trigger, rate, start, end, fade, loop, rec, pre, fadeRec, fadePre, recRun, recOffset);
	}

	init { arg ... theInputs;
		inputs = theInputs;
		// FIXME: get number of channels in buffer
		^this.initOutputs(3, rate);
	}
}