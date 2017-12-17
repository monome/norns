CutFadeVoice : MultiOutUGen {
	// output is 3+ channels: #phase_rd, trig, audio...
    *ar { arg bufnum=0, in, trigger=0, rate_rd=1.0, rate_wr=1.0, start=0.0, end=1.0, fade=0.1, loop_rd=1, loop_wr=0, pre=0;
         ^this.multiNew('audio', bufnum, in, trigger, rate_rd, rate_wr, start, end, fade, loop_rd, loop_wr, pre)
    }

	init { arg ... theInputs;
		inputs = theInputs;
		// FIXME: get number of channels in buffer
		^this.initOutputs(3, rate);
	}
}