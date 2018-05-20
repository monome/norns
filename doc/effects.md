# effect description and parameters

## aux effects (reverb) 

this is a variation of the "zita-rev1" plugin by Fons Adriaensen. 
it is an 8x8 FDN reverberator with allpass and damping filters in feedback delay path.

additionally, there are 2 EQ sections using 2nd-order Regalia-Mitra peaking filters.


### parameters:

- *in_delay* : delay before reverberation; [20, 100] milliseconds
- *lf_x* : crossover frequency between low and middle bands; [50, 1000] hz
- *low_rt60* : time to decay by 60dB in low band; [1, 8] seconds
- *mid_rt60* : time to decay by 60dB in high band; [1, 8] seconds
- *hf_damping* : frequency at which high band decay time is 1/2 of mid band decay time; [1500, (nyquist)] hz
- *eq1_freq* : center frequency EQ section 1; [40, 2500] hz
- *eq1_level* : peak level of EQ section 1 (dB); [-15, 15] dB
- *eq2_freq* : center frequency EQ section 2; [160, 10000] hz
- *eq2_level* : peak level of EQ section 2; [-15, 15] hz
- *dry_wet_mix* : dry/wet mix; [-1, 1] (unitless)
- *level* : output level; [-70, 40] dB

## insert effects (compressor)

a soft-knee stereo compressor. knee smoothing is implemented here by installing a one-pole filter on the amplitude follower, which has the effect of making attack and release times somewhat dependent on input level. the convergence time of the knee smoother is hard-coded to 1/2 of the attack time.

this is a "true" stereo compressor in that the sum of the input channel amplitudes determines the gain reduction amount for both channels.

### parameters:

- *bypass* : ratio of dry to wet signal ; [0, 1] unitless
- *ratio* : compression ratio; for each N dB increase in input level above threshold, output level increases by 1dB; [1, 20] unitless
- *threshold* : amplitude above which the signal is compressed; [-100, 10] dB
- *attack* : time constant (1/e smoothing time) for compression gain to exponentially approach a new _lower_ target level; [1, 1000] ms
- *release* : time constant (1/e smoothing time) for compression gain to exponentially approach a new _higher_ target level; [1, 1000] ms
- *makeup_gain* : gain applied after compression, to make up for lost level; [-96, 96] dB
