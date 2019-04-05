// this is now just an OSC bridge to the crone process;
// we should eliminate it soon and use a different IPC

CroneEffects {

	// input -> aux send level and pan
	*set_in_aux_db {
		arg chan, val; 
		Crone.croneAddr.sendMsg("/set/level/monitor_aux", val.dbamp);
	}

	*set_in_aux_pan { arg chan, val; 
		// FIXME? not supported yet
	}

	// output -> aux send level (stereo)
	*set_out_aux_db { arg val; 
		Crone.croneAddr.sendMsg("/set/level/ext_aux", val.dbamp);
	}

	// aux return level
	*set_aux_return_db { arg val;
		Crone.croneAddr.sendMsg("/set/level/aux_dac", val.dbamp);
	}

	// set aux synth parameter
	*set_aux_param { arg name, val; 
		Crone.croneAddr.sendMsg("/set/param/reverb/"++name, val);
	}

	// enable / disable aux processing
	*aux_enable  { 
		Crone.croneAddr.sendMsg("/set/enabled/reverb", 1.0);
	}

	*aux_disable {
		Crone.croneAddr.sendMsg("/set/enabled/reverb", 0.0);
	}

	// set insert mix
	*set_ins_wet_mix { arg val; 
		Crone.croneAddr.sendMsg("/set/level/ins_mix", val);
	}

	// enable / disable insert processing
	*ins_enable  { 
		Crone.croneAddr.sendMsg("/set/enabled/compressor", 1.0);
	}

	*ins_disable { 
		Crone.croneAddr.sendMsg("/set/enabled/compressor", 0.0);
	}

	// set insert synth parameter
	*set_ins_param { arg name, val; 
		Crone.croneAddr.sendMsg("/set/param/compressor/"++name, val);
	}
}
