
~path = "/home/emb/snd/samples/PAD-PolygonzPad1.wav";

// basic read (reference operation)
(
b = Buffer.readChannel(s, ~path, 0, -1, [0], action: {
	arg bee;
	bee.postln;
	Post << "read " << bee.numFrames << " frames";
	{ bee.plot; }.defer;
});
)

b.free;

// allocate and copy
(
b = Buffer.alloc(s, s.sampleRate * 20, completionMessage: {
	arg thebuf;
	BufUtil.copyChannel(thebuf, ~path, start: 10.0, dur:5.0, action: {
		arg yebuf;
		{ yebuf.plot; }.defer;
	});
});
)