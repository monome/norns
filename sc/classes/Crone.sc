/*

Crone.sc

place for "audio engine" classes


*/


// chopping up samples app.
// glues grid i/o to audio objects
ChopEngine {
	classvar <> server;

	// class init
	*init {
		arg server_;
		server = server_;

	}

	// instance creation
	*new { arg target;
		^super.new.init(target)
	}

}

ChopDefs {
}

// single voice
ChopVoice {
	classvar <> buf_len = 16.0;
	classvar <> buf_frames;

	var <> server; // server
	var <> target; // target (server or node)
	var <> buf; // audio buffer
	var <> trig; // trigger bus
	var <> syn; // playback synth
	var <> pos; // position in soundfile (seconds)
	var <> t; // timestamp of last interaction

	// class init

	// instance creation
	*new { arg target, server;
		^super.new.init(target, server)
	}

	// instance init
	init {
		arg target_, server_;
		if (server_.isNil, { server = Server.default; }, { server = server_; });
		if (target_.isNil, { target = server; }, { target = target_; });
		buf_frames = server.sampleRate * buf_len;
		buf = Buffer.alloc(server, buf_frames, 1);
		trig = Array.fill(2, { Bus.control(server) });
		syn = Array.fill(2, { |i|
			Synth.new(\proto_play_fade, [
				\buf, buf.bufnum,
				\end, buf_frames,
				\amp, 0.0,
				\trig, trig[i].index
			], target)
		});
	}
}

ChopSeq {

}