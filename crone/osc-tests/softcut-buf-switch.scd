
// run after "softcut.scd"
// voices 0 and 1 are both running, on their default buffer assignments

// voice 0 and 1 are on different buffers,
// so setting them to overlapping loop regions should be clickless...
(
n.sendMsg("/set/param/cut/loop_start", 0, 2);
n.sendMsg("/set/param/cut/loop_end", 0, 2.3);
n.sendMsg("/set/param/cut/loop_start", 1, 1.8);
n.sendMsg("/set/param/cut/loop_end", 1, 2.2);
)

// ... but setting them to the same buffer should produce clicks...
n.sendMsg("/set/param/cut/buffer", 1, 0);

// ... flipping voice 0 back to buf 1 should be ok again
n.sendMsg("/set/param/cut/buffer", 0, 1);

