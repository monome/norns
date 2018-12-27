declare name "EqSection";
	
import("stdfaust.lib");
import("effect.lib");
import("filter.lib");	

par_eq = _,_ : par(i, 2, fi.low_shelf(ll,fl) : fi.peak_eq(lp,fp,bp) : fi.high_shelf(lh,fh)) : _,_
with{
  	eq_group(x) = hgroup("[0] parametric EQ",x);
	ls_group(x) = eq_group(vgroup("[1] low shelf", x));

	ll = ls_group(hslider("[0] lf_gain [unit:db]
		[tooltip: amount of low-frequency boost or cut in decibels]",0,-40,40,0.1));
	fl = ls_group(hslider("[1] lf_fc [unit:hz] [style:knob] [scale:log]
		[tooltip: transition-frequency from boost (cut) to unity gain]",200,1,5000,1));

	pq_group(x) = eq_group(vgroup("[2] peaking EQ",x));
	lp = pq_group(hslider("[0] peak_gain [unit:db] [tooltip: amount of
		local boost or cut in decibels]",0,-40,40,0.1));
	fp = pq_group(hslider("[1] peak_freq [unit:hz] [scale:log] [tooltip: peak
		frequency]",49,1,100,1)) : si.smooth(0.999)
		: ba.pianokey2hz;
	q = pq_group(hslider("[2] peak_q [scale:log] [tooltip: quality factor
		(q) of the peak = center-frequency/bandwidth]",40,1,1000,0.1));

	bp = fp/q;

	hs_group(x) = eq_group(vgroup("[3] high shelf]",x));
	lh = hs_group(hslider("[0] high boost|cut [unit:db] [style:knob] [tooltip: amount of
		high-frequency boost or cut in decibels]",0,-40,40,.1));
	fh = hs_group(hslider("[1] transition frequency [unit:hz] [style:knob] [scale:log]
	[tooltip: transition-frequency from boost (cut) to unity gain]",8000,20,10000,1));
};


tilt = _,_ : par(i,2, fi.spectral_tilt(O,f0,bw,alpha) ) : _,_
with{
  O = 5;
  tilt_group(x) = hgroup("[0] spectral tilt",x);
  alpha = tilt_group(hslider("[1] slope [tooltip: slope of spectral tilt across band]",-1/2,-1,1,0.001));
  f0 = tilt_group(hslider("[2] fc [unit:Hz] [tooltip: band start frequency]",100,20,10000,1));
  bw = tilt_group(hslider("[3] bw [unit:Hz] [tooltip: band width",5000,100,10000,1));
};

process = tilt : par_eq;
