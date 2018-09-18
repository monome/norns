// FIXME: order of arguments is not at all well thought out
SoftCutHead : MultiOutUGen {
	// output is 3+ channels: #phase_rd, trig, audio...
    *ar { arg bufnum,
		in=0, rec=0, pre=0, rate=1.0,
		pos=0.0, trig=0, start=0.0, end=1.0, fade=0.1, loop=1,
		fadeRec=0.0, fadePre=1.0, recRun=0, recOffset= -4;
		
		^this.multiNew('audio', bufnum,
			in, rec, pre, rate,
			pos, trig, start, end, fade, loop,
			fadeRec, fadePre, recRun, recOffset);
	}

	init { arg ... theInputs;
		inputs = theInputs;
		// FIXME: get number of channels in buffer
		//
		^this.initOutputs(3, rate);
	}
}