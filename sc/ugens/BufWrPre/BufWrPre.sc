BufWrPre : UGen {
	*ar { arg inputArray, bufnum=0, phase=0.0, pre=0.0, loop=1.0;
		^this.multiNewList(['audio', bufnum, phase,
			pre, loop] ++ inputArray.asArray)

	}
	*kr { arg inputArray, bufnum=0, phase=0.0, pre=0.0, loop=1.0;
		^this.multiNewList(['control', bufnum, phase,
			pre, loop] ++ inputArray.asArray)
	}
	checkInputs {
		if (rate == 'audio' and: {inputs.at(1).rate != 'audio'}, {
			^("phase input is not audio rate: " + inputs.at(1) + inputs.at(1).rate);
		});
		^this.checkValidInputs
	}
}
