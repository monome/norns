VocerVoice {
	var <buf, <syn, <in_bus;
	classvar <fftSize;
	
	*initClass {
		fftSize = 2048;
		StartUp.add {
			var vocerDef = SynthDef.new(\vocer, {
				arg buf, in, out, amp=0.125, gate=0,
				freeze=0, wipe=0, shift=0, scale=1,
				atk=0.01, rel=0.01, pan=0.0;
				
				var snd, chain, chain_phase, aenv, snd_out;
				snd = In.ar(in);
				chain = FFT(LocalBuf(fftSize, 1), snd);
				chain = PV_MagFreeze(chain, freeze);
				chain = PV_MagShift(chain, scale, shift, 0);
				chain = PV_BrickWall(chain, wipe);
				aenv = EnvGen.kr(Env.asr(atk, 1, rel), gate:gate);
				snd_out = IFFT(chain);
				Out.ar(out, Pan2.ar(snd_out, pan, amp * aenv));
			});

			
			CroneDefs.add(vocerDef);
		}
	}

	*new { arg server, output, target;
		^super.new.init(server, output, target);
	}

	init { arg server, out, target;
		in_bus = Bus.audio(server, 1);
		buf = Buffer.alloc(server, fftSize, 1, {
			arg thebuf;
			postln(thebuf);
			syn = Synth.new(\vocer, [\buf, thebuf, \in, in_bus.index, \out, out.index], target:target);
		});
	}

	free {
		
		postln("VocerVoice: freeing");
		buf.free;
		syn.free;
	}
}