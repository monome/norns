n = NetAddr("localhost", 9999);

// compressor wet/dry mix
n.sendMsg("/set/level/ins_mix", 1.0);
n.sendMsg("/set/level/ins_mix", 0.7);
n.sendMsg("/set/level/ins_mix", 0.0);
n.sendMsg("/set/enabled/compressor", 1.0);


// reverb on, ADC -> reverb
n.sendMsg("/set/level/aux_dac", 1.0);
n.sendMsg("/set/level/monitor_aux", 1.0);
n.sendMsg("/set/enabled/reverb", 1);
