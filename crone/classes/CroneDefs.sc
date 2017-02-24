// utility class to load all synthdefs required by crone engines.
CroneDefs {

	*sendDefs { arg s;

		
		// single read head with fade in/out trigger
		SynthDef.new(\play_fade, {
			arg out=0, buf=0,
			start=0, trig=0, rate=1.0, loop=0,
			gate, fade_time=0.2, fade_shape=0.0;

			var snd, aenv, phase;
			
			phase = Sweep.ar(InTrig.kr(trig), BufSampleRate.kr(buf) * rate);
			snd =  BufRd.ar(1, buf, phase + start, loop:loop);
			
			// FIXME: varlag seems broken;
			// replace Lag with EnvGen for arbitrary curves... 
			aenv = Lag.ar(K2A.ar(gate), fade_time);
			Out.ar(out, (snd * aenv));
		}).send(s);

		// mono->stereo patch with pan, smoothing, mute switch
		SynthDef.new(\patch_pan, {
			arg in, out,
			level=1.0, mute=1.0, pan=0.0, 
			amplag=0.01, panlag=0.01;
			var ampenv = Lag.ar(K2A.ar(level * mute), amplag);
			var panenv = Lag.ar(K2A.ar(pan), panlag);
			Out.ar(out, Pan2.ar(In.ar(in) * ampenv, panenv));
		}).send(s);

		// raw mono adc input
		SynthDef.new(\adc, { |in, out| Out.ar(out, SoundIn.ar(in)) }).send(s);
	}
}