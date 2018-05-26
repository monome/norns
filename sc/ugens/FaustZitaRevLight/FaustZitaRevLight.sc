FaustZitaVerbLight : MultiOutUGen
{
  *ar { | in1, in2, reverb_pre_delay(20.0), low_band_cutoff_frequency(200.0), w60db_decay_time_for_low_band(1.0), w60db_decay_time_for_middle_b(1.0), damping_frequency(6000.0) |
      ^this.multiNew('audio', in1, in2, reverb_pre_delay, low_band_cutoff_frequency, w60db_decay_time_for_low_band, w60db_decay_time_for_middle_b, damping_frequency)
  }

  *kr { | in1, in2, reverb_pre_delay(20.0), low_band_cutoff_frequency(200.0), w60db_decay_time_for_low_band(1.0), w60db_decay_time_for_middle_b(1.0), damping_frequency(6000.0) |
      ^this.multiNew('control', in1, in2, reverb_pre_delay, low_band_cutoff_frequency, w60db_decay_time_for_low_band, w60db_decay_time_for_middle_b, damping_frequency)
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

  name { ^"FaustZitaVerbLight" }
}

