n = NetAddr("localhost", 9999);


// "master" I/O levels
n.sendMsg("/set/level/adc", 1.0);
n.sendMsg("/set/level/dac", 1.0);

// ADC monitor level. this is _before_ the aux effects send
n.sendMsg("/set/level/monitor", 1.0);

p = "/home/emb/snd/brown.aif";

// test playback
n.sendMsg("/set/level/tape", 1.0);
n.sendMsg("/tape/play/open", p);
n.sendMsg("/tape/play/start");

n.sendMsg("/tape/play/stop");
