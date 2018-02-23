VariHead : UGen {
	*ar { arg bufnum=0, in=0, rate=1.0, pre=0.0, loop=1.0, start=0, end=1.0;
		^this.multiNew('audio', bufnum, in, rate, pre, loop, start, end);
	}
	init { arg ... theInputs;
	    inputs = theInputs;
	    // no output (yet)
	}
}