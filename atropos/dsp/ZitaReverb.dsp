declare name "ZitaReverb";

import("stdfaust.lib");

process = re.zita_rev1_stereo(pre_del, lf_x, hf_damp, low_rt60, mid_rt60, fsmax)
with{
    fsmax = 48000.0;
	  pre_del = vslider("[1] pre_del [unit:ms] [tooltip: reverb pre-delay ]", 20, 0, 200, 1);
	  lf_x = vslider("[2] lf_fc [unit:Hz] [scale:log] [tooltip: low band cutoff frequency ] ", 200, 30, 1200, 1);
	  low_rt60 = vslider("[3] low_rt60 [unit:s] [tooltip: -60db decay time for low band ]", 1, 0.1, 3, 0.1);
	  mid_rt60 = vslider("[4] mid_rt60 [unit:s] [tooltip: -60db decay time for middle band ]", 1, 0.1, 3, 0.1);
	  hf_damp = vslider("[5] hf_damp [unit:Hz] [tooltip: damping frequency (decay time is 1/2 mid) ] [scale:log]", 6000, 1200, 0.49*fsmax, 1);

};
