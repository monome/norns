n = NetAddr("localhost", 9999);

n.sendMsg("/softcut/buffer/read", "/home/emb/snd/flinflop.aif", 4, 4, -1, 0);

n.sendMsg("/softcut/buffer/clear", 4.2, 0.4);