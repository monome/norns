VariHead : UGen {
	*ar { arg bufnum=0, in=0, rate=1.0, start=0.0, end=1.0;
		^this.multiNew('audio', bufnum, in, rate, start, end);
	}
	init { arg ... theInputs;
	    inputs = theInputs;
	    // no output (yet)
	}
}