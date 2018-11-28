AudioTaper {
	//... TODO?
}

// quick-and-dirty reverse audio taper for VU meters
ReverseAudioTaper {
	classvar <n;
	classvar dbstep;  // step size in db
	classvar breakdb;  // lin/exp breakpoint in db
	classvar breaklin;
	classvar breakn;  // break index

	classvar nlog;
	classvar nlin;

	classvar dbarr; // table of log-distributed amplitudes
	classvar lininc; // discrimination step for linear amplitudes
	classvar nlininc;
	classvar linlogarr; // reverse lookup table

	*initClass {
		/// FIXME? for now, this is a static class and these are hardcoded
		// the tables are a little too large to make lots of these
		n = 64;
		dbstep = 1.0;
		breakdb = -60.0;

		breaklin = breakdb.dbamp;
		breakn = ((n-1) + (breakdb/dbstep)).floor; // break index

		nlog = n - breakn;
		nlin = breakn;

		dbarr = Array.fill(nlog, { arg i;
			(breakdb + (dbstep * i)).dbamp
		});

		// when we get a magnitude, we want to quantize it in the linear domain,
		// then use that value to look up the db bin
		// so, construct a reverse lookup for linear increments...
		// this might be overkill, but it gives us the smallest step between db bins

		lininc = (breakdb+dbstep).dbamp - breakdb.dbamp;
		nlininc = ((1.0 - breaklin) / lininc).floor;
		linlogarr = Array.fill(nlininc, { arg i;
			var amp = breaklin + (i* lininc);
			dbarr.minIndex({ arg val; (amp-val).abs });
		});
	}

	*lookup { arg amp;
		if(amp < breaklin, {
			^(amp / breaklin * (breakn-1)).floor.asInteger;
		}, {
			^linlogarr[((amp - breaklin) / lininc).floor.min(nlininc-1)].asInteger;
		})
	}
}