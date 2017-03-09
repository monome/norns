// utility class to load all synthdefs required by crone engines.
CroneDefs {
    *sendDefs { arg s;
        
        // single read head with fade in/out trigger
        SynthDef.new(\play_fade, {
            arg out=0, buf=0,
            start=0, trig=0, rate=1.0, loop=0,
            gate, fade_time=0.2, fade_shape=0.0,
            mute=0, level=1.0;
            var snd, aenv, amp, phase;          
            phase = Sweep.ar(InTrig.kr(trig), BufSampleRate.kr(buf) * rate);
            snd =  BufRd.ar(1, buf, phase + start, loop:loop);
            aenv = Env.asr(fade_time, 1.0, fade_time, fade_shape);
            amp = EnvGen.ar(aenv, gate);
            amp = amp * Lag.ar(K2A.ar(level * (1 - mute)));         
            Out.ar(out, (snd * amp));
        }).send(s);

        // mono patch with smoothing
        SynthDef.new(\patch_mono, {
            arg in, out, level=1.0, lag=0.01;
            var ampenv = Lag.ar(K2A.ar(level), lag);
            Out.ar(out, In.ar(in) * ampenv);
        }).send(s);
        
        // mono patch with smoothing and feedback
        // (InFeedback introduces 1 audio block of delay)
        SynthDef.new(\patch_mono_fb, {
            arg in, out, level=1.0, lag=0.01;
            var ampenv = Lag.ar(K2A.ar(level), lag);
            Out.ar(out, InFeedback.ar(in) * ampenv);
        }).send(s);

        // record with some level smoothing
        SynthDef.new(\rec_smooth, {
            arg buf, in, offset=0, rec=1, pre=0, lag=0.01,
            run=1, loop=0, trig=0, done=0;
            var ins, pres, recs;            
            ins = In.ar(in);
            pres = Lag.ar(K2A.ar(pre), lag);
            recs = Lag.ar(K2A.ar(rec), lag);
            RecordBuf.ar(ins, buf,
                recLevel:rec, preLevel:pre,
                offset:offset, trigger: InTrig.kr(trig),
                loop:0, doneAction: done);
        }).send(s);
        
        // raw mono adc input
        SynthDef.new(\adc, { |in, out| Out.ar(out, SoundIn.ar(in)) }).send(s);
        
    }
    
}