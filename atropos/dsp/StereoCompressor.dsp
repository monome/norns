declare name "StereoCompressor";

import("stdfaust.lib");
import("effect.lib");
import("filter.lib");

ratio = vslider("[1] ratio", 4.0, 1, 100.0, 0.1);
thresh = vslider("[2] threshold [unit:dB]", -6.0, -40.0, 0, 0.5);
atk = vslider("[3] attack [unit:s] [scale:log]", 0.01, 0.001, 4.0, 0.001);
rel = vslider("[4] release [unit:s] [scale:log]", 0.1, 0.001, 4.0, 0.001);

preGain = vslider("[5] preGain [unit:dB]", 0.0, -24, 24.0, 0.5) : db2linear;
postGain = vslider("[6] postGain [unit:dB]", 0.0, -24, 24.0, 0.5) : db2linear;

postGainStereo(x,y) = x*postGain, y*postGain;
preGainStereo(x,y) = x*preGain, y*preGain;

process = _,_ : preGainStereo : co.compressor_stereo(ratio, thresh, atk, rel) : postGainStereo : _,_;
