n = NetAddr("localhost", 9999);


// "ext" level (input from supercollider, pd or &c.) also pre-send
n.sendMsg("/set/level/ins_mix", 1.0);

// "ext" level (input from supercollider, pd or &c.) also pre-send
n.sendMsg("/set/level/ins_mix", 1.0);

// quit
n.sendMsg("/goodbye");