SerialGrid {
	var <>p; // serial port
	var <>q; // rx data queue
	var <>r; // rx read loop

	// responder functions
	var <>keyUp, <>keyDown;

	*new { arg path = '/dev/ttyUSB0', baud= 115200;
		^super.new.init(path, baud);
	}

	init { arg path, baud;

		if(SerialPort.devices.collect({|str|str.asSymbol}).includes(path.asSymbol), {

			p = SerialPort.new(path, baud);
			q = List.new;

			r = Routine { var ch; inf.do {
				ch = p.read; // blocks here
				q.add(ch);
				this.parse;
			} }.play;

			p.doneAction = { r.stop; };
		}, {
			postln("SerialGrid error: device not found at path "++path);
		});

		keyDown = { |x,y| postln("keyDown: " ++ x ++ " , " ++ y); };
		keyUp = { |x,y| postln("keyUp: " ++ x ++ " , " ++ y); };
	}

	parse {
		if(q.size >= 3, {
			if(q[0] == 33, { keyDown.value(q[1], q[2]); }, {
				if(q[0] == 32, { keyUp.value(q[1], q[2]); }, {
					postln("SerialGrid: unrecognized command");
				});
			});
			q.clear;
		});
	}

	send { arg data;
		fork {
			p.putAll(data);
		};
	}


	led_set { arg x, y, val=true;
		this.send(Int8Array.with( val.if({ 0x11 }, { 0x10 }), x, y  ));
	}

	led_all { arg val=true;
		this.send( val.if({ 0x13 }, { 0x12 }) );
	}

	led_map { arg x, y, data = Int8Array([0, 0, 0, 0, 0, 0, 0, 0]);
		this.send( Int8Array.with(0x14, x, y) ++ data );
	}

	led_row { arg x, y, data = 0x0;
		this.send( Int8Array.with(0x15, x, y, data ) );
	}

	led_col { arg x, y, data = 0x0;
		this.send( Int8Array.with(0x16, x, y, data ) );
	}

	led_intensity{ arg data = 0x0;
		this.send( Int8Array.with(0x17, data ) );
	}

	led_level_set { arg x, y, data = 0;
		this.send( Int8Array.with(0x18, x, y, data) );
	}

	led_level_all { arg val = 0x01;
		this.send( Int8Array.with(0x19, val) );
	}

	pack_nybbles {
		arg data;
		var packed = Int8Array.fill(data.size / 2, {
			arg i;
			(data[i*2] << 4 ) | (data[i*2 + 1] & 0x0f)
		});
		^packed
	}

	led_level_map { arg x, y, data = Int8Array([
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0
	]);
		this.send( Int8Array.with(0x1A, x, y) ++ this.pack_nybbles(data) );
	}

	led_level_row { arg x, y, data = Int8Array([0, 0, 0, 0, 0, 0, 0, 0]);
		this.send( Int8Array.with(0x1B, x, y) ++ this.pack_nybbles(data) );
	}

	led_level_col { arg x, y, data = Int8Array([0, 0, 0, 0, 0, 0, 0, 0]);
		this.send( Int8Array.with(0x1C, x, y) ++ this.pack_nybbles(data) );
	}

}


// GUI emulator

SerialGridEmu {
	var <>win; // window
	var <>but; // buttons

	// responder functions
	var <>keyUp, <>keyDown;

	*new { arg bounds;
		^super.new.init(bounds);
	}

	init { arg bounds=Rect(0, 0, 640, 640);

		var butw = (bounds.width / 16) - 2;
		var buth = (bounds.height/ 16) - 2;

		keyDown = { |x,y| postln("keyDown: " ++ x ++ " , " ++ y); };
		keyUp = { |x,y| postln("keyUp: " ++ x ++ " , " ++ y); };

		win = Window.new(bounds);
		win.view.decorator = FlowLayout(bounds, 0@0, 2@2 );
		but = Array.fill(16, { |i| Array.fill(16, { |j|
			Button(win, butw@buth).action_({  arg but;
				if(but.value == 1, {
					this.keyDown.value(i, j);
				}, {
					this.keyUp.value(i, j);
				});
			});
		}) });

	}


	led_set { arg x, y, val=true;
//		this.send(Int8Array.with( val.if({ 0x11 }, { 0x10 }), x, y  ));
	}

	led_all { arg val=true;
//		this.send( val.if({ 0x13 }, { 0x12 }) );
	}

	led_map { arg x, y, data = Int8Array([0, 0, 0, 0, 0, 0, 0, 0]);
//		this.send( Int8Array.with(0x14, x, y) ++ data );
	}

	led_row { arg x, y, data = 0x0;
//		this.send( Int8Array.with(0x15, x, y, data ) );
	}

	led_col { arg x, y, data = 0x0;
//		this.send( Int8Array.with(0x16, x, y, data ) );
	}

	led_intensity{ arg data = 0x0;
//		this.send( Int8Array.with(0x17, data ) );
	}

	led_level_set { arg x, y, data = 0;
//		this.send( Int8Array.with(0x18, x, y, data) );
	}

	led_level_all { arg val = 0x01;
//		this.send( Int8Array.with(0x19, val) );
	}


	led_level_map { arg x, y, data = Int8Array([
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0
	]);
		//this.send( Int8Array.with(0x1A, x, y) ++ this.pack_nybbles(data) );
	}

	led_level_row { arg x, y, data = Int8Array([0, 0, 0, 0, 0, 0, 0, 0]);
		//this.send( Int8Array.with(0x1B, x, y) ++ this.pack_nybbles(data) );
	}

	led_level_col { arg x, y, data = Int8Array([0, 0, 0, 0, 0, 0, 0, 0]);
		//this.send( Int8Array.with(0x1C, x, y) ++ this.pack_nybbles(data) );
	}

}



