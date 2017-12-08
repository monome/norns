CutFadeLoop : UGen {
    *ar { arg bufnum=0, trigger=0, rate=1.0, start=0.0, end=1.0, fade=0.1, loop=1;
         ^this.multiNew('audio', bufnum, trigger, rate, start, end, fade, loop)
    }
}