// class for repeatedly sending the result of some function

ReportThread {
	// a Routine
	var r;
	// a time interval2
	var t;
	// a function to evaluate
	var f;
	
	*new { arg func = {}, time=0.1;
		^super.new.init(func, time);
	}

	init { arg func, time;
		f = func;
		t = time;
		r = Routine { inf.do {
			func.value;
		}}
	}

	start {
		r.play;
	}

	stop {
		r.stop;
	}

	setTime { arg time;
		t = time;
	}
}