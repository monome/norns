FaustZitaRev : MultiOutUGen
{
  *ar { | in1, in2, in_delay(60.0), lf_x(200.0), low_rt60(3.0), mid_rt60(2.0), hf_damping(6000.0), eq1_freq(315.0), eq1_level(0.0), eq2_freq(1500.0), eq2_level(0.0), dry_wet_mix(0.0), level(-20.0) |
      ^this.multiNew('audio', in1, in2, in_delay, lf_x, low_rt60, mid_rt60, hf_damping, eq1_freq, eq1_level, eq2_freq, eq2_level, dry_wet_mix, level)
  }

  *kr { | in1, in2, in_delay(60.0), lf_x(200.0), low_rt60(3.0), mid_rt60(2.0), hf_damping(6000.0), eq1_freq(315.0), eq1_level(0.0), eq2_freq(1500.0), eq2_level(0.0), dry_wet_mix(0.0), level(-20.0) |
      ^this.multiNew('control', in1, in2, in_delay, lf_x, low_rt60, mid_rt60, hf_damping, eq1_freq, eq1_level, eq2_freq, eq2_level, dry_wet_mix, level)
  } 

  checkInputs {
    if (rate == 'audio', {
      2.do({|i|
        if (inputs.at(i).rate != 'audio', {
          ^(" input at index " + i + "(" + inputs.at(i) + 
            ") is not audio rate");
        });
      });
    });
    ^this.checkValidInputs
  }

  init { | ... theInputs |
      inputs = theInputs
      ^this.initOutputs(2, rate)
  }

  name { ^"FaustZitaRev" }
}

