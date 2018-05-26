TrigPhasor : MultiOutUGen {
	*ar { arg reset, resetPos=0, start=0, end=0, step;
		^this.multiNew('audio', reset, resetPos, start, end, step);
	}

	*kr { arg reset, resetPos=0, start=0, end=0, step;
		^this.multiNew('control', reset, resetPos, start, end, step);
	}

	init { arg ... theInputs;
		inputs = theInputs;
		^this.initOutputs(2, rate);
	}
}
