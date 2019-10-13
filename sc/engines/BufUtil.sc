// collection of static utilities for buffer manipulation
// most of these complete asynchronously and accept a callback argument
BufUtil {

	// destructive trim to new duration.
	// arguments are in seconds
	// if end < -1, use buffer bound
	*trim {
		arg buf, action={}, start=0, end = -1, server=nil;
		var startsamp, endsamp, samps, newbuf;
		if(server == nil, { server = Crone.context.server; });
		startsamp = (start * buf.sampleRate).min(buf.numFrames);
		if(end < -1, {
			endsamp = buf.numFrames;
		}, {
			endsamp = (end * buf.sampleRate).min(buf.numFrames);
		});
		samps = endsamp - startsamp;
		newbuf = Buffer.alloc(server, samps, 1, {
			buf.copyData(newbuf, 0, startsamp, samps);
			buf.free;
			action.value(newbuf);
		});
	}

	// read given channel of given soundfile to buffer, replacing buffer
	// FIXME: should limit max frames allocated, or something
	*readChannel {
		arg buf, path, action={}, channel=0, server=nil;

		if(server == nil, { server = Crone.context.server; });
		Buffer.readChannel(server, path, 0, -1, [channel], {
			arg newbuf;
			buf.free;
			action.value(newbuf);
		});
	}

	// copy given channel of given soundfile to buffer at the indicated position
	// negative duration reads as much of the buffer as possbile
	*copyChannel {
		arg buf, path, action={}, start=0, dur= -1, channel=0, server=nil;

		var newbuf, frstart, frend, numfr;
		if(server == nil, { server = Crone.context.server; });

		Buffer.readChannel(server, path, 0, -1, [channel], {
			arg newbuf;
			Post << "frames in dst buffer: " << buf.numFrames << "\n";
			frstart = buf.sampleRate * start;
			numfr = dur * buf.sampleRate;
			numfr = min(buf.numFrames - frstart, numfr);
			Post << "copying " << numfr << " frames at " << frstart << "\n";
			newbuf.copyData(buf, frstart, 0, numfr);
			newbuf.free;
			action.value(buf);
		});

	}

	// write channel (same as Buffer method, but arguments in seconds)
	*write { arg buf, path, start, dur;
		var numfr = dur * buf.sampleRate;
		var frstart = start * buf.sampleRate;
		buf.write(path, "wav", "float", numfr.asInt, frstart.asInt);
	}


}
