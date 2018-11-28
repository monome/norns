// CroneEngine_FM7
// A Frequency Modulation synth model
Engine_FM7 : CroneEngine {
  classvar <polyDef;
  classvar <paramDefaults;
  classvar <maxNumVoices;

  var <ctlBus;
  var <gr;
  var <voices;

  *initClass {
    maxNumVoices = 16;
    StartUp.add {
      polyDef = SynthDef.new(\polyFM7, {
        // args for whole instrument
        arg out, amp=0.2, amplag=0.02, gate=1, hz,
        // operator frequency multiplier. these can be partials or custom intervals
        hz1=1, hz2=2, hz3=0, hz4=0, hz5=0, hz6=0,
        // operator amplitudes
        amp1=1,amp2=0.5,amp3=0.3,amp4=1,amp5=1,amp6=1,
        // operator phases
        phase1=0,phase2=0,phase3=0,phase4=0,phase5=0,phase6=0,
        // phase modulation params
        hz1_to_hz1=0, hz1_to_hz2=0, hz1_to_hz3=0, hz1_to_hz4=0, hz1_to_hz5=0, hz1_to_hz6=0,
        hz2_to_hz1=0, hz2_to_hz2=0, hz2_to_hz3=0, hz2_to_hz4=0, hz2_to_hz5=0, hz2_to_hz6=0,
        hz3_to_hz1=0, hz3_to_hz2=0, hz3_to_hz3=0, hz3_to_hz4=0, hz3_to_hz5=0, hz3_to_hz6=0,
        hz4_to_hz1=0, hz4_to_hz2=0, hz4_to_hz3=0, hz4_to_hz4=0, hz4_to_hz5=0, hz4_to_hz6=0,
        hz5_to_hz1=0, hz5_to_hz2=0, hz5_to_hz3=0, hz5_to_hz4=0, hz5_to_hz5=0, hz5_to_hz6=0,
        hz6_to_hz1=0, hz6_to_hz2=0, hz6_to_hz3=0, hz6_to_hz4=0, hz6_to_hz5=0, hz6_to_hz6=0,
	// boolean if the carrier is output
	carrier1=1,carrier2=1,carrier3=0,carrier4=0,carrier5=0,carrier6=0,
	// operator amplitude envelopes
	opAmpA1=0.05, opAmpD1=0.1, opAmpS1=1.0, opAmpR1=1.0, opAmpCurve1= -1.0,
	opAmpA2=0.05, opAmpD2=0.1, opAmpS2=1.0, opAmpR2=1.0, opAmpCurve2= -1.0,
	opAmpA3=0.05, opAmpD3=0.1, opAmpS3=1.0, opAmpR3=1.0, opAmpCurve3= -1.0,
	opAmpA4=0.05, opAmpD4=0.1, opAmpS4=1.0, opAmpR4=1.0, opAmpCurve4= -1.0,
	opAmpA5=0.05, opAmpD5=0.1, opAmpS5=1.0, opAmpR5=1.0, opAmpCurve5= -1.0,
	opAmpA6=0.05, opAmpD6=0.1, opAmpS6=1.0, opAmpR6=1.0, opAmpCurve6= -1.0;

        var ctrls, mods, osc, op_env, chans, chan_vec, osc_mix, opEnv1, opEnv2, opEnv3, opEnv4, opEnv5, opEnv6,kilnod;
	opEnv1 = EnvGen.kr(Env.adsr(opAmpA1,opAmpD1,opAmpS1,opAmpR1,1.0, opAmpCurve1),gate,doneAction:0);
	opEnv2 = EnvGen.kr(Env.adsr(opAmpA2,opAmpD2,opAmpS2,opAmpR2,1.0, opAmpCurve2),gate,doneAction:0);
	opEnv3 = EnvGen.kr(Env.adsr(opAmpA3,opAmpD3,opAmpS3,opAmpR3,1.0, opAmpCurve3),gate,doneAction:0);
	opEnv4 = EnvGen.kr(Env.adsr(opAmpA4,opAmpD4,opAmpS4,opAmpR4,1.0, opAmpCurve4),gate,doneAction:0);
	opEnv5 = EnvGen.kr(Env.adsr(opAmpA5,opAmpD5,opAmpS5,opAmpR5,1.0, opAmpCurve5),gate,doneAction:0);
	opEnv6 = EnvGen.kr(Env.adsr(opAmpA6,opAmpD6,opAmpS6,opAmpR6,1.0, opAmpCurve6),gate,doneAction:0);

        // the 6 oscillators, their frequence, phase and amplitude
	ctrls = [[ Lag.kr(hz * hz1,0.01), phase1, Lag.kr(amp1,0.01) * opEnv1 ],
                 [ Lag.kr(hz * hz2,0.01), phase2, Lag.kr(amp2,0.01) * opEnv2 ],
                 [ Lag.kr(hz * hz3,0.01), phase3, Lag.kr(amp3,0.01) * opEnv3 ],
                 [ Lag.kr(hz * hz4,0.01), phase4, Lag.kr(amp4,0.01) * opEnv4 ],
                 [ Lag.kr(hz * hz5,0.01), phase5, Lag.kr(amp5,0.01) * opEnv5 ],
                 [ Lag.kr(hz * hz6,0.01), phase6, Lag.kr(amp6,0.01) * opEnv6 ]];

        // All the operators phase modulation params
        mods = [[hz1_to_hz1, hz2_to_hz1, hz3_to_hz1, hz4_to_hz1, hz5_to_hz1, hz6_to_hz1],
                [hz1_to_hz2, hz2_to_hz2, hz3_to_hz2, hz4_to_hz2, hz5_to_hz2, hz6_to_hz2],
                [hz1_to_hz3, hz2_to_hz3, hz3_to_hz3, hz4_to_hz3, hz5_to_hz3, hz6_to_hz3],
                [hz1_to_hz4, hz2_to_hz4, hz3_to_hz4, hz4_to_hz4, hz5_to_hz4, hz6_to_hz4],
                [hz1_to_hz5, hz2_to_hz5, hz3_to_hz5, hz4_to_hz5, hz5_to_hz5, hz6_to_hz5],
                [hz1_to_hz6, hz2_to_hz6, hz3_to_hz6, hz4_to_hz6, hz5_to_hz6, hz6_to_hz6]];

        // returns a six channel array of OutputProxy objects
        osc = FM7.ar(ctrls,mods);
        // clever h4x to pick which operator is output to audio bus
        chan_vec = [carrier1,carrier2,carrier3,carrier4,carrier5,carrier6];
        osc_mix = Mix.new(chan_vec.collect({|v,i| osc[i]*v}));
        amp = Lag.ar(K2A.ar(amp), amplag);
	kilnod = DetectSilence.ar(osc_mix, 0.01, 0.2, doneAction:2);
        Out.ar(out, (osc_mix * amp).dup);
      });

      // Tell Crone about our SynthDef
      CroneDefs.add(polyDef);

      paramDefaults = Dictionary.with(
        \amp -> -12.dbamp, \amplag -> 0.02,
        \hz1 -> 1, \hz2 -> 2, \hz3 -> 0, \hz4 -> 0, \hz5 -> 0, \hz6 -> 0,
        \amp1 -> 1,\amp2 -> 0.5,\amp3 -> 0.3,\amp -> 1,\amp5 -> 1,\amp6 -> 1,
        \phase1 -> 0,\phase2 -> 0,\phase3 -> 0,\phase4 -> 0,\phase5 -> 0,\phase6 -> 0,
        \ampAtk -> 0.05, \ampDec -> 0.1, \ampSus -> 1.0, \ampRel -> 1.0, \ampCurve -> -1.0,
        \hz1_to_hz1 -> 0, \hz1_to_hz2 -> 0, \hz1_to_hz3 -> 0, \hz1_to_hz4 -> 0, \hz1_to_hz5 -> 0, \hz1_to_hz6 -> 0,
        \hz2_to_hz1 -> 0, \hz2_to_hz2 -> 0, \hz2_to_hz3 -> 0, \hz2_to_hz4 -> 0, \hz2_to_hz5 -> 0, \hz2_to_hz6 -> 0,
        \hz3_to_hz1 -> 0, \hz3_to_hz2 -> 0, \hz3_to_hz3 -> 0, \hz3_to_hz4 -> 0, \hz3_to_hz5 -> 0, \hz3_to_hz6 -> 0,
        \hz4_to_hz1 -> 0, \hz4_to_hz2 -> 0, \hz4_to_hz3 -> 0, \hz4_to_hz4 -> 0, \hz4_to_hz5 -> 0, \hz4_to_hz6 -> 0,
        \hz5_to_hz1 -> 0, \hz5_to_hz2 -> 0, \hz5_to_hz3 -> 0, \hz5_to_hz4 -> 0, \hz5_to_hz5 -> 0, \hz5_to_hz6 -> 0,
        \hz6_to_hz1 -> 0, \hz6_to_hz2 -> 0, \hz6_to_hz3 -> 0, \hz6_to_hz4 -> 0, \hz6_to_hz5 -> 0, \hz6_to_hz6 -> 0,       
	\carrier1 -> 1,\carrier2 -> 1,\carrier3 -> 0,\carrier4 -> 0,\carrier5 -> 0,\carrier6 -> 0,
	\opAmpA1 -> 0.05, \opAmpD1 -> 0.1, \opAmpS1 -> 1.0, \opAmpR1 -> 1.0, \opAmpCurve1 ->  -1.0,
	\opAmpA2 -> 0.05, \opAmpD2 -> 0.1, \opAmpS2 -> 1.0, \opAmpR2 -> 1.0, \opAmpCurve2 ->  -1.0,
	\opAmpA3 -> 0.05, \opAmpD3 -> 0.1, \opAmpS3 -> 1.0, \opAmpR3 -> 1.0, \opAmpCurve3 ->  -1.0,
	\opAmpA4 -> 0.05, \opAmpD4 -> 0.1, \opAmpS4 -> 1.0, \opAmpR4 -> 1.0, \opAmpCurve4 ->  -1.0,
	\opAmpA5 -> 0.05, \opAmpD5 -> 0.1, \opAmpS5 -> 1.0, \opAmpR5 -> 1.0, \opAmpCurve5 ->  -1.0,
	\opAmpA6 -> 0.05, \opAmpD6 -> 0.1, \opAmpS6 -> 1.0, \opAmpR6 -> 1.0, \opAmpCurve6 ->  -1.0;
      );
    }
  }

  *new { arg context, doneCallback;
    ^super.new(context, doneCallback);
  }

  // allocate all the controls and parameters
  alloc {
    // allocate a control group in parallel
    gr = ParGroup.new(context.xg);
    
    // put our voices into a dictionary
    voices = Dictionary.new;
    // put our control bus into a dictionary
    ctlBus = Dictionary.new;

    // loop through all the control names
    polyDef.allControlNames.do({ arg ctl;
      var name = ctl.name;
      postln("control name: " ++ name);
      if((name != \gate) && (name != \hz) && (name != \out), {
        // add this control name to the Bus for the server context
        ctlBus.add(name -> Bus.control(context.server));
        // set this control name to have default value from the first dictionary.
        ctlBus[name].set(paramDefaults[name]);
      });
    });
    ctlBus.postln;

    this.addCommand(\start, "if", { arg msg;
      this.addVoice(msg[1], msg[2], true);
    });

    this.addCommand(\solo, "i", { arg msg;
      this.addVoice(msg[1], msg[2], false);
    });

    this.addCommand(\stop, "i", { arg msg;
      this.removeVoice(msg[1]);
    });

    this.addCommand(\stopAll, "", { 
      gr.set(\gate,0);
      voices.clear;
    });

    // another loop to expose everything in the ctlBus dictionary as a param to Matron
    ctlBus.keys.do({ arg name;
      this.addCommand(name, "f", {arg msg; ctlBus[name].setSynchronous(msg[1]); });
    });
  }

  addVoice { arg id, hz, map=true;
    // the output is the out bus of our client context, the pitch is the value of hz
    var params = List.with(\out, context.out_b.index, \hz, hz);
    var numVoices = voices.size;

    if(voices[id].notNil, {
      voices[id].set(\gate,1);
      voices[id].set(\hz, hz);
    }, { 
      if(numVoices < maxNumVoices, { 
        ctlBus.keys.do({ arg name;
          params.add(name);
          params.add(ctlBus[name].getSynchronous);
        });
        // add a new Synth from our SynthDef into the voices dictionary
        // the doneAction:2 param given to DetectSilence frees the synth implicitly
        voices.add(id -> Synth.new(\polyFM7, params, gr));
        // NodeWatcher informs the client of the server state to get free voice information from there.
        NodeWatcher.register(voices[id]);
        voices[id].onFree({
          voices.removeAt(id);
        });

        if(map, {
          ctlBus.keys.do({ arg name;
            voices[id].map(name, ctlBus[name]);
          });
        });
      });
    });
  }

  removeVoice { arg id;
    if(true, {
      voices[id].set(\gate,0);
    });
  }

  free {
    gr.free;
    ctlBus.do({ arg bus,i; bus.free; });
  }
}
