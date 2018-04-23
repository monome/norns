# softcut

## voice functions

- start  (i) - start voice (play and rec)
- stop  (i) - stop voice (play and rec)
- reset  (i) - reset to `pos`
- set_buf  (ii) - assign voice to use buffer #

## direct control of synth params

- amp  (if) - playback amplitude
- env_time  (if) - specify standard amp envelope for `start`/`stop`
- fade  (if) - fade time for position cut (on `reset`)
- fade_pre  (if) - [default: 1] fade time for pre. 
- fade_rec  (if) - [default: 1] fade time for rec. n.b. "when its ==1, then the crossfade level will be fully applied to the record level during crossfade; when its zero, record level will stay constant during the crossfade."
- loop_end  (if) - specify loop end (ms)
- loop_on  (if) - looping active (0 = off, 1 = on) 
- loop_start  (if) - specify loop start (ms)
- offset  (if)
- pos  (if) - new position, jump with `reset`
- pre  (if) - prerecord level (1.0 = full overdub, 0.0 = full overwrite)
- pre_lag  (if) - smoother for `pre` level
- rate  (if) - speed of playback/record
- rate_lag  (if) - smoother for `rate`
- rec  (if) - record level
- rec_lag  (if) - exponential smoother applied to `rec` level
- rec_on  (if) - record toggle (0 = off, 1 = on)

fade note: "there are different situations where it would make sense to have these at different settings. if you are using it like a delay effect, and constantly looping one section of audio, you might want `fade_rec = 0` to avoid having a dip in volume around the fade point. but then if you change loop positions you might hear a jump in the record level around the former crossfade point."

## routing

- adc_rec  (iif) - adc (i) to voice rec (i) at level (f)
- play_dac  (iif) - playhead (i) to dac channel (i) at level (f)
- play_rec  (iif) - playhead (i) to voice rec (i) at level (f)

## buffer operations

- clear  (i) - clear buffer
- norm  (if) - normalize buffer
- read  (is) - read file into buffer
- trim  (iff) - trim buffer with start (f) and end (f)
- write  (is) - write buffer to file


