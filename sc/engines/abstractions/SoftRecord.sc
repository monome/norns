SoftRecord {

	var <syn; // synth
	var trig_b; // trigger bus
	var <pre; // prerecord level
	var <rec; // record level
	var <state; // \on or \off
	var <pos; // most recent start position
	
	// arg 1: buffer
	// arg 2: mono input bus
	// arg 3: target node 
	*new { arg srv, buf, in, target=nil, action=\addToTail;
		^super.new.init(srv, buf, in, target, action);
	}

	init { arg srv, buf, in, target, action;
		trig_b = Bus.control(srv);
		if(target.isNil, { target = Server.default; });
		syn = Synth.new(\rec_smooth, [
			\buf, buf, \in, in, \run, 0,
			\rec, 0.0, \pre, 1.0, \offset, 0.0,
			\trig, trig_b.index
		], target, action);
		state = \off;
		rec = 1.0;
		pre = 0.0;
		pos = 0;
	}

	// start record.
	// NB: should be called after stop(), otherwise might cause clicks
	start { arg p=nil;
		if(state == \on, {
			postln("warning: restarting SoftRecord without stopping first");
		});
		if(p.notNil, {
			pos = p;
		});
		syn.set(\offset, pos); // jump to ofset
		trig_b.set(1);
		syn.set(\run, 1); // make sure it's running
		syn.set(\rec, rec); // start a fade in
		syn.set(\pre, pre);
		state = \on;
	}

	stop {
		syn.set(\rec, 0.0); // start a fade out
		syn.set(\pre, 1.0);
		state = \off;
		// don't actually ever set run=0 if we want to avoid clicks
		// it's not maximally efficient, but what the heck
	}

	pre_ { arg x;
		pre = x;
		if(state == \on, { syn.set(\pre, pre) });
	}
	
	rec_ { arg x;
		rec = x;
		if(state == \on, { syn.set(\rec, rec) });
	}
}