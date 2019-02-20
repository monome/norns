
// run after "softcut.scd"
// voices 0 and 1 are both running

n.sendMsg("/set/param/cut/rec_flag", 1, 0);
n.sendMsg("/set/param/cut/loop_start", 1, 1);
n.sendMsg("/set/param/cut/loop_end", 1, 2);

// sync voice 1 to voice 0, with small offset.

/*
NB: the offset is necessary if voice 0 (the leader) is writing.
it appears that the follower ends up reading inside the resampling window.
i don't totally get why this is yet - seems like they should both have the same R/W offset.
need a more deterministic sync mechanism
*/

n.sendMsg("/set/param/cut/voice_sync", 1, 0, -10.0 / 48000.0);