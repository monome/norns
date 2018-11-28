NornsSway : Singleton {
	//Carl Testa 2018
	//Special Thanks to Brian Heim, Joshua Parmenter, Chris McDonald, Scott Carver
	classvar <>short_win=1, <>long_win=30, <>refresh_rate=1.0, <>gravity=0.01, <>step=0.05;

	var <>xy, <>quadrant, <>quadrant_names, <>quadrant_map, <>input, <>output, <>analysis_input, <>buffer, <>fftbuffer, <>delaybuffer, <>recorder, <>processing, <>fade=30, <>onsets, <>amplitude, <>clarity, <>flatness, <>amfreq, <>rvmix, <>rvsize, <>rvdamp, <>delaytime, <>delayfeedback, <>delaylatch, <>pbtime, <>pbbend, <>graintrig, <>grainfreq, <>grainpos, <>grainsize, <>granpos, <>granenvspeed, <>granrate, <>filtfreq, <>filtrq, <>freezefreq, <>freezefade, <>panpos, <>pansign, <>analysis_loop, <>above_amp_thresh=false, <>above_clarity_thresh=false, <>above_density_thresh=false, <>amp_thresh=4, <>clarity_thresh=0.6, <>density_thresh=1.5, <>tracker, <>count=0, <>analysis_on=true, <>tracker_on=true, <>audio_processing=true, <>verbose=false, <>polarity=false, <>quadrant_flag=false, <>timelimit=200, <>available_processing, <>all_processing, <>global_change=false;

    init {
		//Setup initial parameters
		this.reset;

		//audio input with chan argument
		input = NodeProxy.audio(Server.default, 1).fadeTime_(fade)
		.source = { |chan=0| In.ar(chan) };

		//fft
		fftbuffer = Buffer.alloc(Server.default, 1024);

		//delaybuffer
		delaybuffer = Buffer.alloc(Server.default, 10*48000, 1);

		//audio recorder
		buffer = Buffer.alloc(Server.default, long_win*48000, 1);
		recorder = NodeProxy.audio(Server.default, 1)
		.source = {
			var off = Lag2.kr(A2K.kr(DetectSilence.ar(input.ar(1), 0.1), 0.3));
            var on = 1-off;
			var fade = MulAdd.new(on, 2, 1.neg);
			var out = XFade2.ar(Silent.ar(), input.ar(1), fade);
			RecordBuf.ar(out, buffer, loop: 1, run: on);
		};

		//this is the placeholder for audio procesing
		processing = NodeProxy.audio(Server.default, 1).fadeTime_(fade)
		.source = { Silent.ar(1) };

		//analysis input so there is option to decouple processed audio from analysed audio
		analysis_input = NodeProxy.audio(Server.default, 1)
		.source = { |chan=0| In.ar(chan) };

		//Build the analysis modules
		this.build_analysis;
		//Begin with an initial nonpolarity mapping of parameters
		this.nonpolarity_map;

		//audio output to listen and change channel
		output = NodeProxy.audio(Server.default, 2)
		.source = { Pan2.ar(processing.ar(1), panpos.kr(1)*pansign.kr(1)); };

		//Longer term analysis controlling placement on processing grid
		analysis_loop = TaskProxy.new({ loop {
			//if analysis on flag is set to true then do analysis
			if(analysis_on==true, {
			//if verbose is on report values
			if(verbose==true,{
			flatness.bus.get({|val|
				(this.name++" flatness: "++val[1]).postln;
					});
			onsets.bus.get({|val|
				(this.name++" onsets: "++val[1]).postln;
					});
			clarity.bus.get({|val|
				(this.name++" clarity: "++val[1]).postln;
					});
				});
			//if signal is above amplitude threshold do analysis
			amplitude.bus.get({|val|
				if(verbose==true,{(this.name++" amp: "++val[1]).postln});
				if( val[1] > amp_thresh, {
					above_amp_thresh=true;
					if(verbose==true,{(this.name++" amp threshold reached").postln});
					clarity.bus.get({|val|
						if( val[1] > clarity_thresh,
							{above_clarity_thresh=true;
							xy[0]=(xy[0]+step).clip(0,1)},
							{above_clarity_thresh=false;
							xy[0]=(xy[0]-step).clip(0,1)});
					});
					onsets.bus.get({|val|
						if( val[1] > density_thresh,
							{above_density_thresh=true;
							xy[1]=(xy[1]+step).clip(0,1)},
							{above_density_thresh=false;
							xy[1]=(xy[1]-step).clip(0,1)});
					});
				}, {
			//else if below threshold drift to center
					above_amp_thresh=false;
					if(verbose==true,{(this.name++" drift to center").postln});
					if(xy[0] > 0.5, {
						(xy[0]=xy[0]-gravity).clip(0,0.5)},{
						(xy[0]=xy[0]+gravity).clip(0,0.5)});
					if(xy[1] > 0.5, {
						(xy[1]=xy[1]-gravity).clip(0,0.5)},{
						(xy[1]=xy[1]+gravity).clip(0,0.5)});
				});
			});
		this.assign_quadrant(xy[0], xy[1]);
		//Checks to see if quadrant has changed, if so, it changes type of processing
		if (quadrant[0] == quadrant[1], {
					},{this.change_processing});
		if (quadrant_flag==true, {
				this.change_processing;
				quadrant_flag=false;
				},{});
		//Tracker processing grid changer is implemented here
			if (tracker_on==true, {
				if( tracker.any({|i,n|i>timelimit}), {//if any item in tracker is above timelimit
					//then choose new processing for that quadrant
					this.choose_new_processing(tracker.detectIndex({|i|i>timelimit}));
					(this.name++": processing grid changing").postln;
					if(verbose==false,{global_change=true;(this.name++": global change enabled").postln});
					quadrant_flag=true;
					tracker[tracker.detectIndex({|i|i>timelimit})]=0;
					//Change polarity for the hell of it
					this.change_polarity;
				},{});
				});
			});
		refresh_rate.wait;
		count=count+1;
		}}).play;
	}

	build_analysis {
		onsets = NodeProxy.control(Server.default, 2)
		.source = {
			//Density Tracker
			var buf = LocalBuf.new(512,1);
			var onsets = Onsets.kr(FFT(buf, analysis_input.ar(1)));
			var shortStats = OnsetStatistics.kr(onsets, short_win);
			var longStats = OnsetStatistics.kr(onsets, long_win);
			var shortValue = (shortStats[0]/short_win);
			var longValue = (longStats[0]/long_win);
			[shortValue, longValue];
		};

		amplitude = NodeProxy.control(Server.default, 2)
		.source = {
			//Amplitude Tracker
			var chain = FFT(LocalBuf(1024), analysis_input.ar(1));
            var loudness = Loudness.kr(chain);
			var shortAverage = AverageOutput.kr(loudness, Impulse.kr(short_win.reciprocal));
			var longAverage = AverageOutput.kr(loudness, Impulse.kr(long_win.reciprocal));
			[shortAverage, longAverage];
		};

		clarity = NodeProxy.control(Server.default, 2)
		.source = {
			var freq, hasFreq, shortAverage, longAverage;
			//Pitch hasfreq Tracker
			# freq, hasFreq = Pitch.kr(analysis_input.ar(1));
            shortAverage = AverageOutput.kr(hasFreq,Impulse.kr(short_win.reciprocal));
			longAverage = AverageOutput.kr(hasFreq,Impulse.kr(long_win.reciprocal));
			[shortAverage, longAverage];
		};

		flatness = NodeProxy.control(Server.default, 2)
		.source = {
			//Spectral Flatness Tracker
			var chain = FFT(LocalBuf(1024), analysis_input.ar(1));
            var flat = SpecFlatness.kr(chain);
			var shortAverage = AverageOutput.kr(flat, Impulse.kr(short_win.reciprocal));
			var longAverage = AverageOutput.kr(flat, Impulse.kr(long_win.reciprocal));
			[shortAverage, longAverage];
		};

		//Parameter Controls
		//amplitude modulation
		amfreq = NodeProxy.control(Server.default, 1).fadeTime_(fade);
		//reverb
		rvmix = NodeProxy.control(Server.default, 1).fadeTime_(fade);
		rvsize = NodeProxy.control(Server.default, 1).fadeTime_(fade);
		rvdamp = NodeProxy.control(Server.default, 1).fadeTime_(fade);
		//delay
		delaytime = NodeProxy.control(Server.default, 1).fadeTime_(fade);
		delayfeedback = NodeProxy.control(Server.default, 1).fadeTime_(fade);
		delaylatch = TaskProxy.new().play;
		//pitch bend
		pbbend = NodeProxy.control(Server.default, 1).fadeTime_(fade);
		pbtime = NodeProxy.control(Server.default, 1).fadeTime_(fade);
		//filter
		filtfreq = NodeProxy.control(Server.default, 1).fadeTime_(fade);
		filtrq = NodeProxy.control(Server.default, 1).fadeTime_(fade);
		//freeze
		freezefreq = NodeProxy.control(Server.default, 1).fadeTime_(fade);
		freezefade = NodeProxy.control(Server.default, 1).fadeTime_(fade);
		//output pan
		panpos = NodeProxy.control(Server.default, 1).fadeTime_(fade);
		pansign = NodeProxy.control(Server.default, 1).fadeTime_(fade);

	}

	nonpolarity_map {
		//TO DO: Modulate the mapping somehow with additional analysis
		//Mapping analysis data to parameter controls
		//amplitude modulation
		amfreq.source = { amplitude.kr(1,0).linlin(0,30,1,14)};
		//reverb
		rvmix.source = { onsets.kr(1,0).linlin(0,6,0.3,1) };
		rvsize.source = { amplitude.kr(1,0).linlin(0,30,0.3,1) };
		rvdamp.source = { clarity.kr(1,0).linlin(0,1,1,0) };
		//delay
		delaytime.source = { onsets.kr(1,0).linexp(0,7,0.5,9) };
		delayfeedback.source = { onsets.kr(1,0).linlin(0,10,0.5,0.05)};
		delaylatch.source = { loop {
			5.0.wait;
			if(0.5.coin, { processing.set(\trigger, 1, \toggle, 1) });
		} };
		//pitch bend
		pbtime.source = { onsets.kr(1,0).linlin(0,6,0.1,1) };
		pbbend.source = { amplitude.kr(1,0).linlin(0,10,0.75,1.5) };
		//filter
		filtfreq.source = { onsets.kr(1,0).linlin(0,6,500,5000) };
		filtrq.source = { amplitude.kr(1,0).linlin(0,10,0.3,0.8) };
		//freeze
		freezefreq.source = { onsets.kr(1,0).linlin(0,6,0.5,6) };
		freezefade.source = { onsets.kr(1,0).linlin(0,6,3,0.3) };
		//pan position
		panpos.source = { amplitude.kr(1,0).linlin(0,10,0,1) };
		pansign.source = { onsets.kr(1,0).linlin(0,6,(-1),1) };


	}

	polarity_map {
		//Mapping analysis data to parameter controls
		//amplitude modulation
		amfreq.source = { amplitude.kr(1,0).linlin(0,30,14,1)};
		//reverb

		rvmix.source = { onsets.kr(1,0).linlin(0,6,0.8,0.5) };
		rvsize.source = { amplitude.kr(1,0).linlin(0,30,0.9,0.7) };
		rvdamp.source = { clarity.kr(1,0).linlin(0,1,0.2,1) };

		//delay
		delaytime.source = { onsets.kr(1,0).linexp(0,7,9,0.5) };
		delayfeedback.source = { onsets.kr(1,0).linlin(0,10,0.05,0.5)};
		delaylatch.source = { loop {
			0.5.wait;
			if(0.5.coin, { processing.set(\trigger, 1, \toggle, 1) });
		} };
		//pitch bend
		pbtime.source = { onsets.kr(1,0).linlin(0,6,1,1.1) };
		pbbend.source = { amplitude.kr(1,0).linlin(0,10,1.5,0.75) };
		//filter
		filtfreq.source = { onsets.kr(1,0).linlin(0,6,5000,500) };
		filtrq.source = { amplitude.kr(1,0).linlin(0,10,0.9,0.3) };
		//freeze
		freezefreq.source = { onsets.kr(1,0).linlin(0,6,6,0.5) };
		freezefade.source = { onsets.kr(1,0).linlin(0,6,0.3,3) };
		//pan position
		panpos.source = { amplitude.kr(1,0).linlin(0,10,1,0) };
		pansign.source = { onsets.kr(1,0).linlin(0,6,1,(-1)) };
	}

	//TO DO: Here is where the different types of processing are defined. I'm wondering if I should separate these out into sub-classes. Would that make it easier to extend it and add processing?
	//Change processing to amplitude modulation
	ampmod {
		//control mapping:
		//amplitude -> freq
		processing.source = {
			var off = Lag2.kr(A2K.kr(DetectSilence.ar(input.ar(1),0.05),0.3));
            var on = 1-off;
			var fade = MulAdd.new(on, 2, 1.neg);
			var out = XFade2.ar(Silent.ar(), input.ar(1), fade);
			var sine = LFTri.ar(amfreq.kr(1), 0).unipolar;
		 	var am = out*sine;
			am;
		 };
		(this.name++": Amplitude Modulation").postln;
	}

	//Change processing to reverb
	reverb {
		//control mapping:
		//onsets -> mix
		//amplitude -> roomsize
		processing.source = { FreeVerb.ar(in: input.ar(1), mix: rvmix.kr(1), room: rvsize.kr(1), damp: rvdamp.kr(1)) };
		(this.name++": Reverb").postln;
		}

	//Change processing to freeze
	freeze {
		//control mapping:
		//onsets -> freezefreq
		//amplitude ->
		processing.source = {
			//First use gate on input
			var off = Lag2.kr(A2K.kr(DetectSilence.ar(input.ar(1),0.05),0.3));
            var on = 1-off;
			var fade = MulAdd.new(on, 2, 1.neg);
			var out = XFade2.ar(Silent.ar(), input.ar(1), fade);
			//then begin freeze
			var freeze;
			var amplitude = Amplitude.kr(out);
			var trig = amplitude > 0.2;
			var gate = Gate.kr(LFClipNoise.kr(freezefreq.kr(1)),trig);
			var chain = FFT(fftbuffer, out);
			chain = PV_Freeze(chain, gate);
			freeze = XFade2.ar(Silent.ar(), IFFT(chain), Lag2.kr(gate,freezefade.kr(1)));
			freeze = FreeVerb.ar(freeze, rvmix.kr(1), rvsize.kr(1));
			freeze = HPF.ar(freeze, 20);
			freeze;
		};
		(this.name++": Freeze").postln;
	}

	//Change processing to delay
	delay {
		//control mapping:
		//onsets -> delaytime
		//onsets -> feedback
		processing.source = {
			var time = Latch.kr(delaytime.kr(1), \trigger.tr);
			var feedback = delayfeedback.kr(1);
			var local = LocalIn.ar(1) + input.ar(1);
			var select = ToggleFF.kr(\toggle.tr(1.neg));
			var delay1 = BufDelayL.ar(delaybuffer, local, Latch.kr(time, 1- select));
			var delay2 = BufDelayL.ar(delaybuffer, local, Latch.kr(time, select));
			var fade = MulAdd.new(Lag2.kr(select, 4), 2, 1.neg);
			var delay = XFade2.ar(delay1, delay2, fade);
			LocalOut.ar(delay * feedback);
			delay;
		};
		(this.name++": Delay").postln;
	}
	//Change processing to pitch bend
	pitchbend {
		//control mapping:
		//onsets -> time
		//amplitude -> bend
		processing.source = {
			PitchShift.ar(input.ar(1), 1, pbbend.kr(1), 0.2, pbtime.kr(1))
		};
		(this.name++": Pitch Bend").postln;
	}

	filter {
		//control mapping:
		//onsets -> filtfreq
		//amplitude -> filtrq
		processing.source = {
			RLPF.ar(input.ar(1), filtfreq.kr(1), filtrq.kr(1)).tanh;
		};
		(this.name++": Filter").postln;
	}

	//change processing to cascading playback
	cascade {
		processing.source = {
			//TO DO: analysis control not implemented
			//pitch ->
			//amp -> number of layers??
			//onsets -> how far back in file does it read? Output current frame maybe?
			var sound = PlayBuf.ar(1, buffer.bufnum, 1, 0, {buffer.numFrames.rand}!16, 1);
			var env = SinOsc.kr(1/16, (0..15).linlin(0,15,8pi.neg,8pi), 0.375);
			var mix = Limiter.ar(Mix.new(sound*env), 1);
			mix;
		};
		(this.name++": Cascade").postln;
	}
	//Silence processing
	silence {
		processing.source = { Silent.ar(1) };
		(this.name++": Silence").postln;
	}

	//execute change in processing type
	change_processing {
		(quadrant_map[quadrant[0]]).value;
	}

	//assign which quadrant source is in based on x/y coordinates
	assign_quadrant { |x, y|
		quadrant = quadrant.shift(1);
		case
		    {(x<0.45) && (y<0.45)} {quadrant.put(0,3);tracker[3]=tracker[3]+1}//quadrant 3
		    {(x>0.55) && (y<0.45)} {quadrant.put(0,4);tracker[4]=tracker[4]+1}//quadrant 4
		    {(x<0.45) && (y>0.55)} {quadrant.put(0,2);tracker[2]=tracker[2]+1}//quadrant 2
		    {(x>0.55) && (y>0.55)} {quadrant.put(0,1);tracker[1]=tracker[1]+1}//quadrant 1
		{(x<0.55) && (x>0.45) && (y<0.55) && (y>0.45)} {quadrant.put(0,0);tracker[0]=tracker[0]+1};//quadrant 0
		//quadrant.postln;
	}

	//map different types of processing to the quadrants using the quadrant names
	map_quadrants {|names|
		names.do({|item,i|
			quadrant_map.put(i,all_processing.at(item));
			available_processing.removeAt(item);
			quadrant_names.put(i,item);
		});
	}

	//map single quadrant
	map_quadrant {|num, name|
		quadrant_map.put(num,all_processing.at(name));
		quadrant_names.put(num,name);
    }

	//change polarity
	change_polarity {
		if(polarity==false, {
			this.polarity_map;polarity=true;
			(this.name++": polarity mapping set").postln;
			},{
			this.nonpolarity_map;polarity=false;
			(this.name++": non-polarity mapping set").postln;
		});
	}

	//fade change
	fade_time { |time|
		//sound
		processing.fadeTime = time;
		//am
		amfreq.fadeTime = time;
		//reverb
		rvmix.fadeTime = time;
		rvsize.fadeTime = time;
		rvdamp.fadeTime = time;
		//delay
		delaytime.fadeTime = time;
		delayfeedback.fadeTime = time;
		//pitch bend
		pbbend.fadeTime = time;
		pbtime.fadeTime = time;
		//filter
		filtfreq.fadeTime = time;
		filtrq.fadeTime = time;
		//freeze
		freezefreq.fadeTime = time;
		freezefade.fadeTime = time;
		//output pan
		panpos.fadeTime = time;
		pansign.fadeTime = time;
	}

	choose_new_processing {|qrant|
		//choose_new_processing function receives a quadrant as an argument and assigns an available processing to that quadrant
		var old, new;
		//don't remap the center, keep it silent
		if(qrant!=0, {
		//get current processing type which is now "old"
		old = quadrant_names[qrant];
		//change processing to one that is available and capture its symbol
		new = available_processing.keys.choose;
		quadrant_map.put(qrant, available_processing.at(new));
		//update quadrant_names
		quadrant_names.put(qrant, new);
		//remove new processing from available
		available_processing.removeAt(new);
		//place old processing in available
		available_processing.put(old, all_processing.at(old));
		},{});
	}

	reset {
		//reset to initial parameters
		//intial placement on processing grid
		xy = [0.5,0.5];//random start
		tracker = [0,0,0,0,0];//number of times in each quadrant area
		//TO DO: the number of data structures I have to keep track of the quadrants and the names of the processing and all the available processing etc feels very clunky. There must be a better way to manage all this information.
		quadrant = Array.newClear(2);
		quadrant_map = Array.newClear(5);
		//change the initial mapping setup here:
		quadrant_names = Array.newClear(5);
		quadrant_names.put(0,1);
		quadrant_names.put(1,2);
		quadrant_names.put(2,8);
		quadrant_names.put(3,3);
		quadrant_names.put(4,4);
		all_processing = Dictionary.new;
		all_processing.put(1, {this.silence});
		all_processing.put(2, {this.delay});
		all_processing.put(3, {this.reverb});
		all_processing.put(4, {this.ampmod});
		all_processing.put(5, {this.pitchbend});
		all_processing.put(6, {this.cascade});
		all_processing.put(7, {this.filter});
		all_processing.put(8, {this.freeze});
		//make all processing currently available
		available_processing = Dictionary.new;
		available_processing.put(1, {this.silence});
		available_processing.put(2, {this.delay});
		available_processing.put(3, {this.reverb});
		available_processing.put(4, {this.ampmod});
		available_processing.put(5, {this.pitchbend});
		available_processing.put(6, {this.cascade});
		available_processing.put(7, {this.filter});
		available_processing.put(8, {this.freeze});
		this.assign_quadrant(xy[0], xy[1]);
		this.map_quadrants(quadrant_names);
		polarity=false;
		global_change=false;
		//quadrant_flag=true;
	}

	end {
		output.free(1);
		input.free(1);
		analysis_input.free(1);
		buffer.free;
		fftbuffer.free;
		delaybuffer.free;
		recorder.free(1);
		processing.free(1);
		onsets.free(1);
		amplitude.free(1);
		clarity.free(1);
		flatness.free(1);
		amfreq.free(1);
		rvmix.free(1);
		rvsize.free(1);
		rvdamp.free(1);
		delaytime.free(1);
		delayfeedback.free(1);
		delaylatch.stop;
		pbtime.free(1);
		pbbend.free(1);
		graintrig.free(1);
		grainfreq.free(1);
		grainpos.free(1);
		grainsize.free(1);
		granrate.free(1);
		granpos.free(1);
		granenvspeed.free(1);
		filtfreq.free(1);
		filtrq.free(1);
		freezefreq.free(1);
		freezefade.free(1);
		panpos.free(1);
		pansign.free(1);
		analysis_loop.stop;
		this.clear;
		//Server.freeAll;
	}

}