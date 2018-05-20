FaustCompressor : MultiOutUGen
{
  *ar { | in1, in2, bypass(0.0), ratio(4.0), threshold(-30.0), attack(5.0), release(50.0), makeup_gain(18.0) |
      ^this.multiNew('audio', in1, in2, bypass, ratio, threshold, attack, release, makeup_gain)
  }

  *kr { | in1, in2, bypass(0.0), ratio(4.0), threshold(-30.0), attack(5.0), release(50.0), makeup_gain(18.0) |
      ^this.multiNew('control', in1, in2, bypass, ratio, threshold, attack, release, makeup_gain)
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

  name { ^"FaustCompressor" }
}