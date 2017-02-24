
Crone_Cutter : CroneEngine {
	classvar nvoices = 4;
	classvar nbufs = 4;
	classvar bufdur = 64.0;
	
	var <s; // server
	var <gr; // groups
	var <bus; // busses
	var <buf; // buffers
	var <v; // voices
	
	*new { arg srv;
		^super.new.initSub(srv);
	}

	initSub { arg srv;
		var com;
		
		s = srv;

		gr = Event.new;
		gr.in = Group.new(s);
		gr.pb = Group.after(gr.in);
		gr.out = Group.after(gr.pb);

		bus = Event.new;
		bus.in = Bus.audio(s, 1);
		bus.pb = Bus.audio(s, 2);

		buf = Array.fill(nbufs, {
			Buffer.alloc(s, s.sampleRate * bufdur);
		});
		
		v = Array.fill(nvoices, { arg i;
			CutFadeVoice.new(s, buf[i % nbufs], gr.pb, gr.out, bus.out);
		});
			
		/// TODO: audio routing, inputs, recording
		
		// build commands list
		com = [
			[\level, \if, { |msg| v[msg[1]].level_(msg[2]) }],
			[\mute, \if, { |msg| v[msg[1]].mute_(msg[2]) }],
			[\pan, \if, { |msg| v[msg[1]].pan_(msg[2]) }],
			[\pos, \if, { |msg| v[msg[1]].pos_(msg[2]) }],
			[\rate, \if, { |msg| v[msg[1]].rate_(msg[2]) }],
			[\read, \is, { |msg| this.readBuf(msg[1], msg[2]) }],
			[\clear, \i, { |msg| this.clearBuf(msg[1]) }],
			[\normalize, \if, { |msg| this.normalizeBuf(msg[1], msg[2]) }],
		];

		com.do({ arg comarr;
			postln("adding command: " ++ comarr);
			this.addCommand(comarr[0], comarr[1], comarr[2]);
		});
	}

	// --- routing
	// play head to output
	// play head to rec head

	// start/stop recording
	record { arg i, state;
		// TODO
	}

	recLevel { arg i, val;
		// TODO
	}

	preLevel { arg i, val;
		// TODO
	}
	
	// clear a buffer
	clearBuf { arg i;
		buf[i].zero;
	}

	// normalize buffer to given max
	normalizeBuf { arg i, x;
		buf[i].normalize(x);
	}
	
	// destructive trim
	trimBuf { arg i, start, end;
		// TODO: any voices using the old buffer
		// need to be reassigned to use the new one...

		/*
		var startsamp, endsamp, samps, newbuf;
		startsamp = start * s.sampleRate;
		endsamp = end * s.sampleRate;
		samps = endsamp - startsamp;
		newbuf = Buffer.alloc(s, samps);
		buf[i].copyData(newbuf, 0, startsamp, samps);
		// reassign...
		buf[i].free;
		*/
	}
	
	// disk read
	readBuf { arg i, path;
		if(i < nbufs && i >= 0, { 
			buf[i].readChannel(path, channels:[0]);
		});
	}
	
	// disk write
	writeBuf { arg i, path;
		if(i < nbufs && i >= 0, { 
			buf[i].write(path, "wav");
		});
		
	}

}
