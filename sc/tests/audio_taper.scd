// quick-and-dirty reverse audio taper for VU meters

n = 64; // how many steps
~dbstep = 1; // step size in db
~breakdb = -60; // lin/exp breakpoint in db
~breaklin = ~breakdb.dbamp;
~breakn = ((n-1) + (~breaklevel/~dbstep)).floor; // break index

~nlog = n - ~breakn;
~nlin = ~breakn;


// table of log-scaled amplitudes
~dbarr = Array.fill(~nlog, { arg i;
	(~breakdb + (~dbstep * i)).dbamp
});

// when we get a magnitude, we want to quantize it in the linear domain,
// then use that value to look up the db bin
// so, construct a reverse lookup for linear increments...
// this might be overkill, but it gives us the smallest step between db bins
~lininc = (~breakdb+~dbstep).dbamp - ~breakdb.dbamp;
~nlininc = ((1.0 - ~breaklin) / ~lininc).floor;
~linlogarr = Array.fill(~nlininc, { arg i;
	var amp = ~breaklin + (i* ~lininc);
	~dbarr.minIndex({ arg val; (amp-val).abs });
});

~linlogarr.plot;

~lookupReverseTaper = { arg amp;
	if(amp < ~breaklin, {
		amp / ~breaklin * (~breakn-1);
	}, {
		~linlogarr[((amp - ~breaklin) / ~lininc).floor]
	})
};



~test = { arg n;
	Array.collect(n, 0, 1.0/n).do({ |x| ~lookupReverseTaper.value(x); }).plot;
};

~test.value(100);