n = NetAddr("localhost", 9999);

// compressor wet/dry mix
n.sendMsg("/set/level/comprssor_mix", 1.0);
n.sendMsg("/set/level/compressor_mix", 0.7);
n.sendMsg("/set/level/compressor_mix", 0.0);
n.sendMsg("/set/enabled/compressor", 1.0);


// reverb on, ADC -> reverb
n.sendMsg("/set/level/rev_dac", 1.0);
n.sendMsg("/set/level/monitor_rev", 1.0);
n.sendMsg("/set/enabled/reverb", 1);
