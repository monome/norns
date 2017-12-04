// crossfaded looping with explicit phasors and boundary triggers

s = Server.local;
s.boot;
s.waitForBoot { Routine {

	SynthDef.new(\play_trig_mono, {
		arg out=0, amp=0.5,
		buf, // buffer
		phase_bus, // output control bus for phase
		loop = 2.0, // loop point in seconds
		rate = 1.0, // playback rate
		start_pos=0.0, // start position in seconds
		start_trig;

		var phase_scale, phase_start, phase_end,
		phase, snd;

		phase_scale = BufRateScale.kr(buf) * SampleRate.ir;
		phase_start = start_pos * phase_scale;
		phase_end = phase_scale * BufDur.kr(buf);
		loop = loop * phase_scale;
		phase = Phasor.ar(start_trig, rate, phase_start, phase_end, phase_start);
		snd = BufRd.ar(1, buf, phase, 0, 2);

		SendTrig.ar(phase > loop, 0, 0.0);
		Out.kr(phase_bus, A2K.kr(phase > loop));
		Out.ar(out, (snd * amp).dup);

	}).send(s);

	s.sync;

	/*
	NetAddr.langPort
	*/

	b = Buffer.read(s, Platform.resourceDir +/+ "sounds/a11wlk01.wav");

	o = OSCFunc({ arg msg, time; msg.postln; }, '/tr', s.addr);

	~phase_bus = Bus.control(s);
	s.sync;

	x = Synth.new(\play_trig_mono, [ \buf, b.bufnum ], s);
	x.set(\amp, -2.dbamp);
	x.set(\phase_bus, ~phase_bus.index);

	/*
	~phase_poll = Routine { inf.do {
		~phase_bus.getSynchronous.postln;
		0.1.wait;
	} }.play;
	*/

}.play; };
	/*
	~phase_poll.stop;
	x.free;
	o.free;
	*/