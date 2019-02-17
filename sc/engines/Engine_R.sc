Engine_R : CroneEngine {
	var modules;
	var engineTopGroup;
	var macros;
	var trace=false;

	*new { |context, callback| ^super.new(context, callback) }

	alloc {
		this.addCommand('new', "ss") { |msg|
			if (trace) {
				[SystemClock.seconds, \newCommand, msg[1], msg[2]].debug(\received);
			};
			this.newCommand(msg[1], msg[2]);
		};

		this.addCommand('delete', "s") { |msg|
			if (trace) {
				[SystemClock.seconds, \deleteCommand, msg[1].asString[0..20]].debug(\received);
			};
			this.deleteCommand(msg[1]);
		};

		this.addCommand('connect', "ss") { |msg|
			if (trace) {
				[SystemClock.seconds, \connectCommand, (msg[1].asString + msg[2].asString)[0..20]].debug(\received);
			};
			this.connectCommand(msg[1], msg[2]);
		};

		this.addCommand('disconnect', "ss") { |msg|
			if (trace) {
				[SystemClock.seconds, \disconnectCommand, (msg[1].asString + msg[2].asString)[0..20]].debug(\received);
			};
			this.disconnectCommand(msg[1], msg[2]);
		};

		this.addCommand('set', "sf") { |msg|
			if (trace) {
				[SystemClock.seconds, \setCommand, (msg[1].asString + msg[2].asString)[0..20]].debug(\received);
			};
			this.setCommand(msg[1], msg[2]);
		};

		this.addCommand('bulkset', "s") { |msg|
			if (trace) {
				[SystemClock.seconds, \bulksetCommand, msg[1].asString[0..20]].debug(\received);
			};
			this.bulksetCommand(msg[1]);
		};

		this.addCommand('newmacro', "ss") { |msg|
			if (trace) {
				[SystemClock.seconds, \newmacroCommand, (msg[1].asString + msg[2].asString)[0..20]].debug(\received);
			};
			this.newmacroCommand(msg[1], msg[2]);
		};

		this.addCommand('deletemacro', "s") { |msg|
			if (trace) {
				[SystemClock.seconds, \deletemacroCommand, (msg[1].asString)[0..20]].debug(\received);
			};
			this.deletemacroCommand(msg[1]);
		};

		this.addCommand('macroset', "sf") { |msg|
			if (trace) {
				[SystemClock.seconds, \macrosetCommand, (msg[1].asString + msg[2].asString)[0..20]].debug(\received);
			};
			this.macrosetCommand(msg[1], msg[2]);
		};

		this.addCommand('trace', "i") { |msg|
			trace = msg[1].asBoolean;
		};

		RModule.allSubclasses do: _.sendDefs;

		engineTopGroup = Group.tail(context.xg);
		context.server.sync;

		macros = ();
		modules = [];
	}

	free {
		this.deleteAllModules;
	}

	deleteAllModules {
		modules.collect(_.name) do: { |modulename| // collect used to dup here since deleteCommand removes entries in modules
			this.deleteCommand(modulename)
		};
	}

	newCommand { |name, kind|
		if (this.lookupRModuleClassByKind(kind.asSymbol).isNil) {
			"unable to create %. invalid module type %".format(name.asString.quote, kind.asString.quote).error;
			^nil
		};
		if (this.lookupModuleByName(name).notNil) {
			"unable to create %. module named % already exists".format(name.asString.quote, name.asString.quote).error;
			^nil
		} {
			var spec = this.getModuleSpec(kind);
			var group = Group.tail(engineTopGroup);
			var patchCordGroup = spec[\inputs].notEmpty.if { Group.tail(group) };
			var processingGroup = Group.tail(group);
			var inbusses = spec[\inputs].collect { |input| input -> Bus.audio }; // TODO: defer allocation / lazily allocate busses
			var outbusses = spec[\outputs].collect { |output| output -> Bus.audio }; // TODO: defer allocation / lazily allocate busses
			var module = (
				name: name.asSymbol, // TODO: validate name, should be [a-zA-Z0-9_]
				kind: kind.asSymbol,
				serverContext: (
					group: group,
					patchCordGroup: patchCordGroup,
					processingGroup: processingGroup,
					inbusses: inbusses,
					outbusses: outbusses,
				),
				patchCords: spec[\inputs].collect { |input| input -> () }.asDict, // TODO: better to bundle this together with inbusses (?)
				instance: this.instantiateModuleClass(kind.asSymbol, processingGroup, inbusses, outbusses)
			);
			modules = modules.add(module);
		};
	}

	connectCommand { |outlet, inlet|
		if (this.isConnected(outlet, inlet)) {
			"outlet % is already connected to inlet %".format(outlet.asString.quote, inlet.asString.quote).error;
		} {
			var sourceModuleRef, output;
			var destModuleRef, input;
			var sourceModule, destModule;
			var sourceModuleIndex, destModuleIndex;

			# sourceModuleRef, output = outlet.asString.split($/);
			// TODO: validate outlet exists against getModuleSpec
			# destModuleRef, input = inlet.asString.split($/);
			// TODO: validate inlet exists against getModuleSpec

			sourceModule = this.lookupModuleByName(sourceModuleRef);

			if (sourceModule.isNil) {
				"module named % not found among modules %".format(sourceModuleRef.quote, modules.collect { |module| module[\name].asString }.join(", ").quote).error;
				^nil
			};

			sourceModuleIndex = modules.indexOf(sourceModule);

			destModule = this.lookupModuleByName(destModuleRef);

			if (destModule.isNil) {
				"module named % not found among modules %".format(destModuleRef.quote, modules.collect { |module| module[\name].asString }.join(", ").quote).error;
				^nil
			};

			destModuleIndex = modules.indexOf(destModule);

			if (this.moduleHasOutputNamed(sourceModule, output).not) {
				"invalid output % for % module named % (possible outputs are %)".format(
					output.asString.quote,
					sourceModule[\kind].asString.quote,
					sourceModule[\name].asString.quote,
					this.getModuleSpec(sourceModule[\kind])[\outputs].collect{ |o| o.asString.quote }.join(", ")
				).error;
			} {
				if (this.moduleHasInputNamed(destModule, input).not) {
					"invalid input % for % module named % (possible inputs are %)".format(
						input.asString.quote,
						destModule[\kind].asString.quote,
						destModule[\name].asString.quote,
						this.getModuleSpec(sourceModule[\kind])[\inputs].collect{ |i| i.asString.quote }.join(", ")
					).error;
				} {
					destModule[\patchCords][input.asSymbol][outlet.asSymbol] = Synth( // TODO: relies on core CroneDefs for patching, refactor to not require Crone
						if (sourceModuleIndex >= destModuleIndex, \patch_mono_fb, \patch_mono),
						[
							\in, sourceModule[\serverContext][\outbusses].detect { |busAssoc| busAssoc.key == output.asSymbol }.value,
							\out, destModule[\serverContext][\inbusses].detect { |busAssoc| busAssoc.key == input.asSymbol }.value,
							\level, 1.0
						],
						destModule[\serverContext][\patchCordGroup],
						\addToTail
					);
				}
			}
		}
	}

	disconnectCommand { |outlet, inlet|
		if (this.isConnected(outlet, inlet)) {
			var sourceModuleRef, output;
			var destModuleRef, input;
			var destModule;

			# sourceModuleRef, output = outlet.asString.split($/);
			// TODO: validate outlet exists against getModuleSpec
			# destModuleRef, input = inlet.asString.split($/);
			// TODO: validate inlet exists against getModuleSpec

			destModule = this.lookupModuleByName(destModuleRef);

			destModule[\patchCords][input.asSymbol][outlet.asSymbol].free;
			destModule[\patchCords][input.asSymbol][outlet.asSymbol] = nil;
		} {
			"outlet % is not connected to inlet %".format(outlet.asString.quote, inlet.asString.quote).error;
		}
	}

	deleteCommand { |name|
		var moduleToDelete = this.lookupModuleByName(name);
		if (moduleToDelete.isNil) {
			"module named % not found among modules %".format(name.quote, modules.collect { |module| module[\name].asString }.join(", ").quote).error;
		} {
			var serverContext = moduleToDelete[\serverContext];

			this.disconnectModulePatchCords(moduleToDelete);

			moduleToDelete[\instance].free;
			serverContext[\group].free;
			serverContext[\inbusses] do: { |inputBusAssoc| inputBusAssoc.value.free };
			serverContext[\outbusses] do: { |outputBusAssoc| outputBusAssoc.value.free };
			modules.remove(moduleToDelete);
		};
	}

	setCommand { |moduleparam, value|
		var moduleRef, parameter, module, spec;

		# moduleRef, parameter = moduleparam.asString.split($.);

		module = this.lookupModuleByName(moduleRef);
		if (module.isNil) {
			"module named % not found among modules %".format(moduleRef.quote, modules.collect { |module| module[\name].asString }.join(", ").quote).error;
		} {
			spec = this.getModuleSpec(module[\kind]);

			if (spec[\parameters].includes(parameter.asSymbol)) {
				module[\instance].set(parameter, value);
			} {
				"parameter % not valid for module named % (kind: %)".format(parameter.asString.quote, moduleRef.asString.quote, module[\kind].asString.quote).error;
			}
		}
	}

	bulksetCommand { |bundle|
		context.server.makeBundle(nil) { // TODO: udp package size limitations and bulksetCommand
			bundle.asString.split($ ).clump(2).do { |cmd, i|
				this.setCommand(cmd[0], cmd[1]);
			}
		}
	}

	newmacroCommand { |name, bundle|
		var macro;
		var bus = Bus.control;

		macro = (
			moduleparams: bundle.asString.split($ ),
			bus: bus
		);

		context.server.makeBundle(nil) {
			macro[\moduleparams].do { |moduleparam|
				var moduleRef, parameter, module, spec;
				# moduleRef, parameter = moduleparam.asString.split($.);

				module = this.lookupModuleByName(moduleRef);
				if (module.isNil) {
					"module named % not found among modules %".format(moduleRef.quote, modules.collect { |module| module[\name].asString }.join(", ").quote).error;
				} {
					spec = this.getModuleSpec(module[\kind]);

					if (spec[\parameters].includes(parameter.asSymbol)) {
						module[\instance].map(parameter, bus);
					} {
						"parameter % not valid for module named % (kind: %)".format(parameter.asString.quote, moduleRef.asString.quote, module[\kind].asString.quote).error;
					}
				}
			}
		};

		macros[name.asSymbol] = macro;
	}

	deletemacroCommand { |name|
		macros[name.asSymbol][\bus].free;
		macros[name.asSymbol][\moduleparams].do {
			// TODO: unmap control
			// Reset to current bus value - or is this needed at all?
		};
		macros[name.asSymbol] = nil;
	}

	macrosetCommand { |name, value|
		// TODO: validate presence of macro
		// TODO: controlSpecs are not checked here! only allow macro creation of params with same controlSpec in newmacro and constrain here?
		macros[name.asSymbol][\bus].set(value);
	}

	moduleHasOutputNamed { |module, name|
		^this.getModuleSpec(module[\kind])[\outputs].any { |output| output == name.asSymbol }
	}

	moduleHasInputNamed { |module, name|
		^this.getModuleSpec(module[\kind])[\inputs].any { |input| input == name.asSymbol }
	}

	isConnected { |outlet, inlet|
		var destModuleRef, input, destModule;
		# destModuleRef, input = inlet.asString.split($/);
		destModule = this.lookupModuleByName(destModuleRef);
		^if (destModule.isNil) {
			"module named % not found among modules %".format(destModuleRef.asString.quote, modules.collect { |module| module[\name].asString }.join(", ").quote).error;
			false;
		} {
			var patchCordsForInlet = destModule[\patchCords][input.asSymbol];
			if (patchCordsForInlet.isNil) {
				"input named % not valid for module named % (kind: %)".format(input.asString.quote, destModuleRef.asString.quote, destModule[\kind].asString.quote).error;
				false;
			} {
				patchCordsForInlet.keys.any { |patchedOutlet| patchedOutlet == outlet.asSymbol };
			}
		}
	}

	disconnectModulePatchCords { |moduleToDisconnect|
		var spec = this.getModuleSpec(moduleToDisconnect[\kind]);
		var moduleInlets = spec[\inputs].collect { |input|
			moduleToDisconnect[\name]++"/"++input.asString
		};
		var moduleOutlets = spec[\outputs].collect { |output|
			moduleToDisconnect[\name]++"/"++output.asString
		};
		[this.getAllOutlets, moduleInlets].allTuples.select { |tuple|
			this.isConnected(tuple[0], tuple[1]);
		} ++ [moduleOutlets, this.getAllInlets].allTuples.select { |tuple|
			this.isConnected(tuple[0], tuple[1]);
		}.do { |tuple|
			this.disconnectCommand(tuple[0], tuple[1]);
		}
	}

	getAllOutlets {
		^modules
			.collect { |module|
				var spec = this.getModuleSpec(module[\kind]);
				spec[\outputs].collect { |output|
					module[\name].asString ++ "/" ++ output.asString
				}.asArray
			}.flatten
	}

	getAllInlets {
		^modules
			.collect { |module|
				var spec = this.getModuleSpec(module[\kind]);
				spec[\inputs].collect { |output|
					module[\name].asString ++ "/" ++ output.asString
				}.asArray
			}.flatten
	}

	lookupModuleByName { |name|
		name = name.asSymbol;
		^modules.detect { |module| module[\name] == name }
	}

	getModuleSpec { |kind|
		^this.lookupRModuleClassByKind(kind.asSymbol) !? _.spec;
	}

	instantiateModuleClass { |kind, processingGroup, inbusses, outbusses|
		^this.lookupRModuleClassByKind(kind).new(context, processingGroup, inbusses, outbusses);
	}

	lookupRModuleClassByKind { |kind|
		^this.class.allRModuleClasses.detect { |rModuleClass| rModuleClass.shortName == kind }
	}

	*generateLuaSpecs {
		^"local specs = {}\n" ++
		"\n" ++
		this.allRModuleClasses.collect { |rModuleClass|
			rModuleClass.generateLuaSpecs
		}.join("\n")
	}

	*generateModulesDocsSection {
		^"## Available Modules\n" ++
		"\n" ++
		this.allRModuleClasses.collect { |rModuleClass|
			rModuleClass.generateModuleDocs
		}.join("\n")
	}

	*allRModuleClasses {
		^RModule.allSubclasses.sort { |a, b|
			a.shortName.asString < b.shortName.asString
		}
	}
}

// Abstract superclass
RModule {
	var synth;
	var context;
	*params { ^nil } // specified as Array of name -> ControlSpec or name -> (Spec: ControlSpec, LagTime: Float) associations where name correspond to SynthDef ugenGraphFunc argument
	*paramControlSpecs {
		^this.params !? { |params|
			params.collect { |paramAssoc|
				var paramAssocValue = paramAssoc.value;
				("param_"++paramAssoc.key).asSymbol -> if (paramAssocValue.class == ControlSpec) {
					paramAssocValue
				} {
					// TODO: Event assumed
					paramAssocValue[\Spec]
				}
			}.asDict
		}
	}
	*paramLagTimes {
		^this.params !? { |params|
			params.collect { |paramAssoc|
				var paramAssocValue = paramAssoc.value;
				("param_"++paramAssoc.key).asSymbol -> if (paramAssocValue.class == Event) {
					paramAssocValue[\LagTime]
				} {
					nil
				}
			}.asDict
		}
	}
	*ugenGraphFunc { ^this.subclassResponsibility(thisMethod) }
	*defName { ^this.asSymbol }
	*sendDefs {
		var defName = this.defName.asSymbol;
		"RModule %: spawning SynthDef %...".format(this.asString.quote, defName.asString.quote).inform;
		SynthDef(
			defName,
			this.ugenGraphFunc,
			this.prGetLagTimes,
			metadata: (specs: this.paramControlSpecs)
		).add;
		"...OK. SynthDef % was sent to server".format(defName.asString.quote).inform;
		"".postln;
	}

	map { |parameter, bus|
		this.class.params.detect { |param| param.key == parameter.asSymbol } !? { |param| // TODO: DRY
			var name;
			name = ("param_"++param.key.asString).asSymbol;
			synth.map(name, bus);
		}
	}

	set { |parameter, value|
		this.class.params.detect { |param| param.key == parameter.asSymbol } !? { |param| // TODO: DRY
			var name, controlSpec, constrainedParamValue;
			name = ("param_"++param.key.asString).asSymbol;
			controlSpec = this.class.paramControlSpecs[name];
			constrainedParamValue = controlSpec.constrain(value);
			synth.set(name, constrainedParamValue);
		}
	}

	*new { |context, processingGroup, inbusses, outbusses|
		^super.new.initRModule(context, processingGroup, inbusses, outbusses);
	}

	initRModule { |argContext, group, inbusses, outbusses|
		context = argContext;
		synth = Synth(
			this.class.defName.asSymbol,
			this.class.prGetDefaultRModuleSynthArgs(inbusses, outbusses),
			target: group
		);
	}

	free { // TODO: typically overridden by subclass if more than one synth is spawned in init { }
		synth.free;
	}

	*spec {
		^(
			inputs: this.prLookupArgNameSuffixesPrefixedBy('in_'),
			outputs: this.prLookupArgNameSuffixesPrefixedBy('out_'),
			parameters: this.prLookupArgNameSuffixesPrefixedBy('param_')
		)
	}

	*prGetLagTimes {
		^this.ugenGraphFunc.def.argNames.collect { |argName|
			if ( argName.asString.beginsWith("param_") ) {
				this.paramLagTimes[argName]
			} { nil }
		};
	}

	*prGetDefaultRModuleSynthArgs { |inbusses, outbusses|
		^(
			this.spec[\inputs].collect { |inputName|
				[
					("in_"++inputName.asString).asSymbol,
					inbusses.detect { |busAssoc| busAssoc.key == inputName.asSymbol }.value
				] // TODO: report error when busAssoc not found
			} ++
			this.spec[\outputs].collect { |outputName|
				[
					("out_"++outputName.asString).asSymbol,
					outbusses.detect { |busAssoc| busAssoc.key == outputName.asSymbol }.value
				] // TODO: report error when busAssoc not found
			} ++
			this.spec[\parameters].collect { |parameterName|
				var name = ("param_"++parameterName.asString).asSymbol;
				var controlSpec = this.paramControlSpecs[name]; // TODO: report error when controlSpec is not found / or rely on .asSpec
				[name, controlSpec.default]
			}
		).flatten
	}

	*prLookupArgNameSuffixesPrefixedBy { |token|
		token = token.asString;
		^this.ugenGraphFunc.def.argNames.select { |argName|
			argName.asString.beginsWith(token)
		}.collect { |argName|
			argName.asString[token.size..].asSymbol
		}
	}

	*generateLuaSpecs {
		^"specs['"++this.shortName.asString++"'] = {\n"++
			if (this.params.isNil) {
				""
			} {
				this.spec[\parameters].collect { |param| // TODO: report error when controlSpec is not found / or rely on .asSpec
					var controlSpec = this.paramControlSpecs[("param_"++param.asString).asSymbol]; // TODO: throw error when nothing found -- will happen when *params does not comply with param_Args in ugenGraphFunc
					"\t" ++ param.asString ++ " = " ++ if (controlSpec.class == Symbol) {
						"\\" ++ controlSpec.asString
					} {
						controlSpec.asSpecifier !? { |specifier| "ControlSpec."++specifier.asString.toUpper } ? ("ControlSpec.new("++[
							switch (controlSpec.minval) { -inf } { "-math.huge" } { inf } { "math.huge" } ? controlSpec.minval,
							switch (controlSpec.maxval) { -inf } { "-math.huge" } { inf } { "math.huge" } ? controlSpec.maxval,
							controlSpec.warp.asSpecifier.asString.quote,
							controlSpec.step,
							switch (controlSpec.default) { -inf } { "-math.huge" } { inf } { "math.huge" } ? controlSpec.default,
							controlSpec.units.quote
						].join(", ")++")")
					}
				}.join(",\n") ++ "\n"
			} ++
		"}" ++ "\n";
	}

	*generateModuleDocs {
		var inputs = this.spec[\inputs];
		var outputs = this.spec[\outputs];
		var params = this.spec[\parameters];

		^"### " ++ this.shortName.asString ++ "\n" ++
		"- Inputs:" ++
		if (
			inputs.isEmpty,
			" None",
			" " ++ inputs.collect { |input|
				"`" ++ input ++ "`"
			}.join(", ")
		) ++ "\n"
		"- Outputs:" ++
		if (
			outputs.isEmpty,
			" None",
			" " ++ outputs.collect { |output|
				"`" ++ output ++ "`"
			}.join(", ")
		) ++ "\n"
		"- Parameters:" ++
		if (
			params.isEmpty,
			" None",
			"\n" ++
			params.collect { |param|
				"\t- `" ++ param ++ "`"
			}.join($\n)
		) ++ "\n"
	}

	*shortName { ^this.subclassResponsibility(thisMethod) } // Preferable <= 8 chars
}

// Status: tested
RSoundInModule : RModule {
	*shortName { ^'SoundIn' }

	*ugenGraphFunc {
		^{
			|
				out_Left,
				out_Right,
				sys_in
			|

 			var in = In.ar(sys_in, 2);
			Out.ar(out_Left, in[0]);
			Out.ar(out_Right, in[1]);
		}
	}

	// override required to add sys_in argument. TODO: or do a synth.set(\sys_in, ...); instead?
	initRModule { |argContext, group, inbusses, outbusses|
		context = argContext;
		synth = Synth(
			this.class.defName.asSymbol,
			(
				this.class.prGetDefaultRModuleSynthArgs(inbusses, outbusses) ++
				[\sys_in, context.in_b] // here
			).flatten,
			target: group
		);
	}
}

// Status: tested
RSoundOutModule : RModule {
	*shortName { ^'SoundOut' }

/*
	TODO
	*params {
		^[
			'Gain' -> \db.asSpec.copy.maxval_(12).default_(0),
		]
	}
*/

	*ugenGraphFunc {
		^{
			|
				in_Left,
				in_Right,
				// TODO param_Gain,
				sys_out
			|

			var amp = 1; // TODO param_Gain.dbamp;
			Out.ar(sys_out, [In.ar(in_Left) * amp, In.ar(in_Right) * amp]);
		}
	}

	// override required to add sys_out argument. TODO: or do a synth.set(\sys_out, ...); instead?
	initRModule { |argContext, group, inbusses, outbusses|
		context = argContext;
		synth = Synth(
			this.class.defName.asSymbol,
			(
				this.class.prGetDefaultRModuleSynthArgs(inbusses, outbusses) ++
				[\sys_out, context.out_b] // here
			).flatten,
			target: group
		);
	}
}

// Status: tested
// Inspiration from A-110 (but no Sync input)
RMultiOscillatorModule : RModule {
	*shortName { ^'MultiOsc' }

	*params {
		^[
			'Range' -> (
				Spec: ControlSpec.new(-2, 2, nil, 1, 0),
				LagTime: 0.01
			),
			'Tune' -> (
				Spec: ControlSpec.new(-600, 600, nil, 0, 0, "cents"),
				LagTime: 0.01
			),
			'PulseWidth' -> (
				Spec: \unipolar.asSpec.copy.default_(0.5),
				LagTime: 0.01
			),
			'FM' -> (
				Spec: \unipolar.asSpec, // TODO: bipolar?
				LagTime: 0.01
			),
			'PWM' -> (
				Spec: \unipolar.asSpec,
				LagTime: 0.01
			),
		]
	}

	*ugenGraphFunc {
		^{
			|
				in_FM,
				in_PWM,
				out_Sine,
				out_Triangle,
				out_Saw,
				out_Pulse,
				param_Range,
				param_Tune,
				param_FM,
				param_PulseWidth,
				param_PWM
			|

			var sig_FM = In.ar(in_FM);
			var sig_PWM = In.ar(in_PWM);

			var fullRange = ControlSpec(12.midicps, 120.midicps);

			var frequency = fullRange.constrain(
				( // TODO: optimization - implement overridable set handlers and do this calculation in sclang rather than server
					3 +
					param_Range +
					(param_Tune / 1200) +
					(sig_FM * 10 * param_FM) // 0.1 = 1 oct
				).octcps
			);

			var pulseWidth = (
				param_PulseWidth + (sig_PWM * param_PWM)
			).clip(0, 1);

			Out.ar(
				out_Sine,
				SinOsc.ar(frequency) * 0.5
			);

			Out.ar(
				out_Triangle,
				LFTri.ar(frequency) * 0.5 // not band limited
			);

			Out.ar(
				out_Saw,
				Saw.ar(frequency) * 0.5
			);

			Out.ar(
				out_Pulse,
				Pulse.ar(frequency, pulseWidth / 2) * 0.5
			);
		}
	}
}

// Status: tested
RSineOscillatorModule : RModule {
	*shortName { ^'SineOsc' }

	*params {
		^[
			'Range' -> (
				Spec: ControlSpec.new(-2, 2, nil, 1, 0),
				LagTime: 0.01
			),
			'Tune' -> (
				Spec: ControlSpec.new(-600, 600, nil, 0, 0, "cents"),
				LagTime: 0.01
			),
			'FM' -> (
				Spec: \unipolar.asSpec, // TODO: bipolar?
				LagTime: 0.01
			),
		]
	}

	*ugenGraphFunc {
		^{
			|
				in_FM,
				out_Out,
				param_Range,
				param_Tune,
				param_FM
			|

			var sig_FM = In.ar(in_FM);

			var fullRange = ControlSpec(12.midicps, 120.midicps);

			var frequency = fullRange.constrain(
				( // TODO: optimization - implement overridable set handlers and do this calculation in sclang rather than server
					3 +
					param_Range +
					(param_Tune / 1200) +
					(sig_FM * 10 * param_FM) // 0.1 = 1 oct
				).octcps
			);

			Out.ar(
				out_Out,
				SinOsc.ar(frequency) * 0.5
			);
		}
	}
}

// Status: tested
RTriangleOscillatorModule : RModule {
	*shortName { ^'TriOsc' }

	*params {
		^[
			'Range' -> (
				Spec: ControlSpec.new(-2, 2, nil, 1, 0),
				LagTime: 0.01
			),
			'Tune' -> (
				Spec: ControlSpec.new(-600, 600, nil, 0, 0, "cents"),
				LagTime: 0.01
			),
			'FM' -> (
				Spec: \unipolar.asSpec, // TODO: bipolar?
				LagTime: 0.01
			),
		]
	}

	*ugenGraphFunc {
		^{
			|
				in_FM,
				out_Out,
				param_Range,
				param_Tune,
				param_FM
			|

			var sig_FM = In.ar(in_FM);

			var fullRange = ControlSpec(12.midicps, 120.midicps);

			var frequency = fullRange.constrain(
				( // TODO: optimization - implement overridable set handlers and do this calculation in sclang rather than server
					3 +
					param_Range +
					(param_Tune / 1200) +
					(sig_FM * 10 * param_FM) // 0.1 = 1 oct
				).octcps
			);

			Out.ar(
				out_Out,
				LFTri.ar(frequency) * 0.5 // TODO: not band-limited
			);
		}
	}
}

// Status: tested
RSawtoothOscillatorModule : RModule {
	*shortName { ^'SawOsc' }

	*params {
		^[
			'Range' -> (
				Spec: ControlSpec.new(-2, 2, nil, 1, 0),
				LagTime: 0.01
			),
			'Tune' -> (
				Spec: ControlSpec.new(-600, 600, nil, 0, 0, "cents"),
				LagTime: 0.01
			),
			'FM' -> (
				Spec: \unipolar.asSpec, // TODO: bipolar?
				LagTime: 0.01
			),
		]
	}

	*ugenGraphFunc {
		^{
			|
				in_FM,
				out_Out,
				param_Range,
				param_Tune,
				param_FM
			|

			var sig_FM = In.ar(in_FM);

			var fullRange = ControlSpec(12.midicps, 120.midicps);

			var frequency = fullRange.constrain(
				( // TODO: optimization - implement overridable set handlers and do this calculation in sclang rather than server
					3 +
					param_Range +
					(param_Tune / 1200) +
					(sig_FM * 10 * param_FM) // 0.1 = 1 oct
				).octcps
			);

			Out.ar(
				out_Out,
				Saw.ar(frequency) * 0.5
			);
		}
	}
}

// Status: tested
RPulseOscModule : RModule {
	*shortName { ^'PulseOsc' }

	*params {
		^[
			'Range' -> (
				Spec: ControlSpec.new(-2, 2, nil, 1, 0),
				LagTime: 0.01
			),
			'Tune' -> (
				Spec: ControlSpec.new(-600, 600, nil, 0, 0, "cents"),
				LagTime: 0.01
			),
			'PulseWidth' -> (
				Spec: \unipolar.asSpec.copy.default_(0.5),
				LagTime: 0.01
			),
			'FM' -> (
				Spec: \unipolar.asSpec, // TODO: bipolar?
				LagTime: 0.01
			),
			'PWM' -> (
				Spec: \unipolar.asSpec.copy.default_(0.4),
				LagTime: 0.01
			),
		]
	}

	*ugenGraphFunc {
		^{
			|
				in_FM,
				in_PWM,
				out_Out,
				param_Range,
				param_Tune,
				param_FM,
				param_PulseWidth,
				param_PWM
			|

			var sig_FM = In.ar(in_FM);
			var sig_PWM = In.ar(in_PWM);

			var fullRange = ControlSpec(12.midicps, 120.midicps);

			var frequency = fullRange.constrain(
				(
					3 +
					param_Range +
					(param_Tune / 1200) +
					(sig_FM * 10 * param_FM) // 0.1 = 1 oct
				).octcps
			);

			var pulseWidth = (
				param_PulseWidth + (sig_PWM * param_PWM)
			).clip(0, 1);

			Out.ar(
				out_Out,
				Pulse.ar(frequency, pulseWidth / 2) * 0.5
			);
		}
	}
}

// Status: partly tested, TODO: test in_Reset together with param_Reset
// Inspiration from A-145
RMultiLFOModule : RModule {
	*shortName { ^'MultiLFO' }

	*params {
		^[
			'Frequency' -> (
				Spec: ControlSpec(0.01, 50, 'exp', 0, 1, "Hz"),
				LagTime: 0.01
			),
			'Reset' -> \unipolar.asSpec.copy.step_(1), // TODO
		]
	}

	*ugenGraphFunc {
		^{
			|
				in_Reset,
				out_InvSaw,
				out_Saw,
				out_Sine,
				out_Triangle,
				out_Pulse,
				param_Frequency,
				param_Reset
			|

			var sig_Reset = In.ar(in_Reset);

			var retrig = (Trig.ar(sig_Reset) + Trig.kr(param_Reset)) > 0; // TODO: remove param?

			var invSawPhase = Phasor.ar(retrig, -1/SampleRate.ir*param_Frequency, 0.5, -0.5, 0.5); // TODO: Retrig in middle of saw ramp?
			var invSawSig = invSawPhase * 0.5; // +- 2.5V

			var sawPhase = Phasor.ar(retrig, 1/SampleRate.ir*param_Frequency, -0.5, 0.5, -0.5); // TODO: Retrig in middle of saw ramp?
			var sawSig = sawPhase * 0.5; // +- 2.5V

			var sinePhase = Phasor.ar(retrig, 1/SampleRate.ir*param_Frequency);
			var sineSig = SinOsc.ar(0, sinePhase.linlin(0, 1, 0, 2pi), 0.25); // +- 2.5V

			var trianglePhase = Phasor.ar(retrig, 1/SampleRate.ir*param_Frequency*2, -0.5, 1.5, -0.5);
			var triangleSig = trianglePhase.fold(-0.5, 0.5) * 0.5; // +- 2.5V

			var pulsePhase = Phasor.ar(retrig, 1/SampleRate.ir*param_Frequency*1, 0, 1, 0);
			var pulseSig = ((pulsePhase < 0.5)*0.5)-0.25; // +- 2.5V

			Out.ar(
				out_InvSaw,
				invSawSig
			);

			Out.ar(
				out_Saw,
				sawSig
			);

			Out.ar(
				out_Sine,
				sineSig
			);

			Out.ar(
				out_Triangle,
				triangleSig
			);

			Out.ar(
				out_Pulse,
				pulseSig
			);
		}
	}
}

// Status: partly tested, TODO: test in_Reset together with param_Reset
// Inspiration from A-145
RSineLFOModule : RModule {
	*shortName { ^'SineLFO' }

	*params {
		^[
			'Frequency' -> (
				Spec: ControlSpec(0.01, 50, 'exp', 0, 1, "Hz"),
				LagTime: 0.01
			),
			'Reset' -> \unipolar.asSpec.copy.step_(1), // TODO
		]
	}

	*ugenGraphFunc {
		^{
			|
				in_Reset,
				out_Out,
				param_Frequency,
				param_Reset
			|

			var sig_Reset = In.ar(in_Reset);

			var retrig = (Trig.ar(sig_Reset) + Trig.kr(param_Reset)) > 0; // TODO: remove param?

			var sinePhase = Phasor.ar(retrig, 1/SampleRate.ir*param_Frequency);
			var sineSig = SinOsc.ar(0, sinePhase.linlin(0, 1, 0, 2pi), 0.25); // +- 2.5V

			Out.ar(
				out_Out,
				sineSig
			);
		}
	}
}
// Status: tested
RLinMixerModule : RModule {
	*shortName { ^'LinMixer' }

	*params {
		^[
			'In1' -> (
				Spec: \unipolar.asSpec,
				LagTime: 0.1
			),
			'In2' -> (
				Spec: \unipolar.asSpec,
				LagTime: 0.1
			),
			'In3' -> (
				Spec: \unipolar.asSpec,
				LagTime: 0.1
			),
			'In4' -> (
				Spec: \unipolar.asSpec,
				LagTime: 0.1
			),
			'Out' -> (
				Spec: \unipolar.asSpec,
				LagTime: 0.1
			)
		]
	}

	*ugenGraphFunc {
		^{
			|
				in_In1,
				in_In2,
				in_In3,
				in_In4,
				out_Out,
				param_In1,
				param_In2,
				param_In3,
				param_In4,
				param_Out
			|

			var sig_In1 = In.ar(in_In1);
			var sig_In2 = In.ar(in_In2);
			var sig_In3 = In.ar(in_In3);
			var sig_In4 = In.ar(in_In4);

			Out.ar(
				out_Out,
				(
					(sig_In1 * param_In1) +
					(sig_In2 * param_In2) +
					(sig_In3 * param_In3) +
					(sig_In4 * param_In4)
				) * param_Out
			);
		}
	}
}

// Status: untested
RDbMixerModule : RModule {
	*shortName { ^'DbMixer' }

	*params {
		^[
			'In1' -> (
				Spec: \db.asSpec,
				LagTime: 0
			),
			'In2' -> (
				Spec: \db.asSpec,
				LagTime: 0
			),
			'In3' -> (
				Spec: \db.asSpec,
				LagTime: 0
			),
			'In4' -> (
				Spec: \db.asSpec,
				LagTime: 0
			),
			'Out' -> (
				Spec: \db.asSpec,
				LagTime: 0
			)
		]
	}

	*ugenGraphFunc {
		^{
			|
				in_In1,
				in_In2,
				in_In3,
				in_In4,
				out_Out,
				param_In1,
				param_In2,
				param_In3,
				param_In4,
				param_Out
			|

			var sig_In1 = In.ar(in_In1); // TODO: Add lag here instead
			var sig_In2 = In.ar(in_In2);
			var sig_In3 = In.ar(in_In3);
			var sig_In4 = In.ar(in_In4);

			Out.ar(
				out_Out,
				(
					(sig_In1 * param_In1.dbamp) +
					(sig_In2 * param_In2.dbamp) +
					(sig_In3 * param_In3.dbamp) +
					(sig_In4 * param_In4.dbamp)
				) * param_Out.dbamp
			);
		}
	}
}

// Status: partly tested, exp mode needs more testing
// Inspiration from A-130/A-131
RDAmplifierModule : RModule {
	*shortName { ^'Amp2' }

	*params {
		^[
			'Gain' -> (
				Spec: \unipolar.asSpec,
				LagTime: 0.1
			),
			'GainModulation' -> (
				Spec: \bipolar.asSpec,
				LagTime: 0.1
			),
			'In1' -> (
				Spec: \unipolar.asSpec,
				LagTime: 0.1
			),
			'In2' -> (
				Spec: \unipolar.asSpec,
				LagTime: 0.1
			),
			'Out' -> (
				Spec: \unipolar.asSpec,
				LagTime: 0.1
			),
			'Mode' -> ControlSpec.new(0, 1, 'lin', 1, 0),
		]
	}

	*ugenGraphFunc {
		^{
			|
				in_GainModulation,
				in_In1,
				in_In2,
				out_Out,
				param_Gain,
				param_GainModulation,
				param_In1,
				param_In2,
				param_Out,
				param_Mode
			|

			var sig_In1 = In.ar(in_In1);
			var sig_In2 = In.ar(in_In2);
			var sig_GainModulation = In.ar(in_GainModulation); // TODO: bipolar unmap

			var curveSpec = ControlSpec.new(0, 1, 13.81523);

			var gainGain = SelectX.kr(param_Mode, [param_Gain, curveSpec.map(param_Gain)]); // TODO: exp hack
			var in1Gain = SelectX.kr(param_Mode, [param_In1, curveSpec.map(param_In1)]); // TODO: exp hack
			var in2Gain = SelectX.kr(param_Mode, [param_In2, curveSpec.map(param_In2)]); // TODO: exp hack
			var outGain = SelectX.kr(param_Mode, [param_Out, curveSpec.map(param_Out)]); // TODO: exp hack

			var inMix = (sig_In1 * in1Gain) + (sig_In2 * in2Gain);

			Out.ar(
				out_Out,
				inMix * (gainGain + (sig_GainModulation * param_GainModulation)) * outGain
			);
		}
	}
}

// Status: tested
RAmplifierModule : RModule {
	*shortName { ^'Amp' }

	*params {
		^[
			'Level' -> (
				Spec: \unipolar.asSpec,
				LagTime: 0.1
			)
		]
	}

	*ugenGraphFunc {
		^{
			|
				in_Exp,
				in_Lin,
				in_In,
				out_Out,
				param_Level
			|

			var sig_Exp = In.ar(in_Exp);
			var sig_Lin = In.ar(in_Lin);
			var sig_In = In.ar(in_In);

			var curveSpec = ControlSpec.new(0, 1, 13.81523); // TODO: exp approximation, dunno about this?

			Out.ar(
				out_Out,
				sig_In * (param_Level + sig_Lin.clip(0, 0.5) + curveSpec.map(sig_Exp).clip(0, 0.5)).clip(0, 0.5)
			);
		}
	}
}

// Status: tested
// Inspiration from A-121
RSVFMultiModeFilterModule : RModule {
	*shortName { ^'MMFilter' }

	*params {
		^[
			'AudioLevel' -> (
				Spec: \amp.asSpec,
				LagTime: 0.1
			),
			'Frequency' -> (
				Spec: \widefreq.asSpec,
				LagTime: 0.1
			),
			'Resonance' -> (
				Spec: \unipolar.asSpec,
				LagTime: 0.1
			),
			'FM' -> (
				Spec: \bipolar.asSpec,
				LagTime: 0.1
			),
			'ResonanceModulation' -> (
				Spec: \bipolar.asSpec,
				LagTime: 0.1
			),
		]
	}

	*ugenGraphFunc {
		^{
			|
				in_In,
				in_FM,
				in_ResonanceModulation,
				out_Notch,
				out_Highpass,
				out_Bandpass,
				out_Lowpass,
				param_AudioLevel,
				param_Frequency,
				param_Resonance,
				param_FM,
				param_ResonanceModulation
			|

			var sig_In = In.ar(in_In);
			var sig_FM = In.ar(in_FM);
			var sig_ResonanceModulation = In.ar(in_ResonanceModulation);

			var frequencySpec = \widefreq.asSpec;
			var resonanceSpec = \unipolar.asSpec;

			var frequency = frequencySpec.map(frequencySpec.unmap(param_Frequency) + (sig_FM * param_FM));
			var resonance = resonanceSpec.map(resonanceSpec.unmap(param_Resonance) + (sig_ResonanceModulation * param_ResonanceModulation));

			Out.ar(
				out_Notch,
				SVF.ar(
					sig_In * param_AudioLevel,
					frequency,
					resonance,
					lowpass: 0,
					bandpass: 0,
					highpass: 0,
					notch: 1,
					peak: 0
				)
			);
			Out.ar(
				out_Highpass,
				SVF.ar(
					sig_In,
					frequency,
					resonance,
					lowpass: 0,
					bandpass: 0,
					highpass: 1,
					notch: 0,
					peak: 0
				)
			);
			Out.ar(
				out_Bandpass,
				SVF.ar(
					sig_In,
					frequency,
					resonance,
					lowpass: 0,
					bandpass: 1,
					highpass: 0,
					notch: 0,
					peak: 0
				)
			);
			Out.ar(
				out_Lowpass,
				SVF.ar(
					sig_In,
					frequency,
					resonance,
					lowpass: 1,
					bandpass: 0,
					highpass: 0,
					notch: 0,
					peak: 0
				)
			);
		}
	}
}

// Status: tested
RSVFLowpassFilterModule : RModule {
	*shortName { ^'LPFilter' }

	*params {
		^[
			'AudioLevel' -> (
				Spec: \amp.asSpec,
				LagTime: 0.1
			),
			'Frequency' -> (
				Spec: \widefreq.asSpec,
				LagTime: 0.1
			),
			'Resonance' -> (
				Spec: \unipolar.asSpec,
				LagTime: 0.1
			),
			'FM' -> (
				Spec: \bipolar.asSpec,
				LagTime: 0.1
			),
			'ResonanceModulation' -> (
				Spec: \bipolar.asSpec,
				LagTime: 0.1
			),
		]
	}

	*ugenGraphFunc {
		^{
			|
				in_In,
				in_FM,
				in_ResonanceModulation,
				out_Out,
				param_AudioLevel,
				param_Frequency,
				param_Resonance,
				param_FM,
				param_ResonanceModulation
			|

			var sig_In = In.ar(in_In);
			var sig_FM = In.ar(in_FM);
			var sig_ResonanceModulation = In.ar(in_ResonanceModulation);

			var frequencySpec = \widefreq.asSpec;
			var resonanceSpec = \unipolar.asSpec;

			var frequency = frequencySpec.map(frequencySpec.unmap(param_Frequency) + (sig_FM * param_FM));
			var resonance = resonanceSpec.map(resonanceSpec.unmap(param_Resonance) + (sig_ResonanceModulation * param_ResonanceModulation));

			Out.ar(
				out_Out,
				SVF.ar(
					sig_In * param_AudioLevel,
					frequency,
					resonance,
					lowpass: 1,
					bandpass: 0,
					highpass: 0,
					notch: 0,
					peak: 0
				)
			);
		}
	}
}

// Status: tested
RSVFHighpassFilterModule : RModule {
	*shortName { ^'HPFilter' }

	*params {
		^[
			'AudioLevel' -> (
				Spec: \amp.asSpec,
				LagTime: 0.1
			),
			'Frequency' -> (
				Spec: \widefreq.asSpec,
				LagTime: 0.1
			),
			'Resonance' -> (
				Spec: \unipolar.asSpec,
				LagTime: 0.1
			),
			'FM' -> (
				Spec: \bipolar.asSpec,
				LagTime: 0.1
			),
			'ResonanceModulation' -> (
				Spec: \bipolar.asSpec,
				LagTime: 0.1
			),
		]
	}

	*ugenGraphFunc {
		^{
			|
				in_In,
				in_FM,
				in_ResonanceModulation,
				out_Out,
				param_AudioLevel,
				param_Frequency,
				param_Resonance,
				param_FM,
				param_ResonanceModulation
			|

			var sig_In = In.ar(in_In);
			var sig_FM = In.ar(in_FM);
			var sig_ResonanceModulation = In.ar(in_ResonanceModulation);

			var frequencySpec = \widefreq.asSpec;
			var resonanceSpec = \unipolar.asSpec;

			var frequency = frequencySpec.map(frequencySpec.unmap(param_Frequency) + (sig_FM * param_FM));
			var resonance = resonanceSpec.map(resonanceSpec.unmap(param_Resonance) + (sig_ResonanceModulation * param_ResonanceModulation));

			Out.ar(
				out_Out,
				SVF.ar(
					sig_In * param_AudioLevel,
					frequency,
					resonance,
					lowpass: 0,
					bandpass: 0,
					highpass: 1,
					notch: 0,
					peak: 0
				)
			);
		}
	}
}

// Status: tested
RSVFBandpassFilterModule : RModule {
	*shortName { ^'BPFilter' }

	*params {
		^[
			'AudioLevel' -> (
				Spec: \amp.asSpec,
				LagTime: 0.1
			),
			'Frequency' -> (
				Spec: \widefreq.asSpec,
				LagTime: 0.1
			),
			'Resonance' -> (
				Spec: \unipolar.asSpec,
				LagTime: 0.1
			),
			'FM' -> (
				Spec: \bipolar.asSpec,
				LagTime: 0.1
			),
			'ResonanceModulation' -> (
				Spec: \bipolar.asSpec,
				LagTime: 0.1
			),
		]
	}

	*ugenGraphFunc {
		^{
			|
				in_In,
				in_FM,
				in_ResonanceModulation,
				out_Out,
				param_AudioLevel,
				param_Frequency,
				param_Resonance,
				param_FM,
				param_ResonanceModulation
			|

			var sig_In = In.ar(in_In);
			var sig_FM = In.ar(in_FM);
			var sig_ResonanceModulation = In.ar(in_ResonanceModulation);

			var frequencySpec = \widefreq.asSpec;
			var resonanceSpec = \unipolar.asSpec;

			var frequency = frequencySpec.map(frequencySpec.unmap(param_Frequency) + (sig_FM * param_FM));
			var resonance = resonanceSpec.map(resonanceSpec.unmap(param_Resonance) + (sig_ResonanceModulation * param_ResonanceModulation));

			Out.ar(
				out_Out,
				SVF.ar(
					sig_In * param_AudioLevel,
					frequency,
					resonance,
					lowpass: 0,
					bandpass: 1,
					highpass: 0,
					notch: 0,
					peak: 0
				)
			);
		}
	}
}

// Status: tested
RSVFBandrejectFilterModule : RModule {
	*shortName { ^'BRFilter' }

	*params {
		^[
			'AudioLevel' -> (
				Spec: \amp.asSpec,
				LagTime: 0.1
			),
			'Frequency' -> (
				Spec: \widefreq.asSpec,
				LagTime: 0.1
			),
			'Resonance' -> (
				Spec: \unipolar.asSpec,
				LagTime: 0.1
			),
			'FM' -> (
				Spec: \bipolar.asSpec,
				LagTime: 0.1
			),
			'ResonanceModulation' -> (
				Spec: \bipolar.asSpec,
				LagTime: 0.1
			),
		]
	}

	*ugenGraphFunc {
		^{
			|
				in_In,
				in_FM,
				in_ResonanceModulation,
				out_Out,
				param_AudioLevel,
				param_Frequency,
				param_Resonance,
				param_FM,
				param_ResonanceModulation
			|

			var sig_In = In.ar(in_In);
			var sig_FM = In.ar(in_FM);
			var sig_ResonanceModulation = In.ar(in_ResonanceModulation);

			var frequencySpec = \widefreq.asSpec;
			var resonanceSpec = \unipolar.asSpec;

			var frequency = frequencySpec.map(frequencySpec.unmap(param_Frequency) + (sig_FM * param_FM));
			var resonance = resonanceSpec.map(resonanceSpec.unmap(param_Resonance) + (sig_ResonanceModulation * param_ResonanceModulation));

			Out.ar(
				out_Out,
				SVF.ar(
					sig_In * param_AudioLevel,
					frequency,
					resonance,
					lowpass: 0,
					bandpass: 0,
					highpass: 0,
					notch: 1,
					peak: 0
				)
			);
		}
	}
}

// Status: tested
RLadderLowpassFilterModule : RModule {
	*shortName { ^'LPLadder' }

	*params {
		^[
			'Frequency' -> (
				Spec: \widefreq.asSpec,
				LagTime: 0.1
			),
			'Resonance' -> (
				Spec: \unipolar.asSpec,
				LagTime: 0.1
			),
			'FM' -> (
				Spec: \bipolar.asSpec,
				LagTime: 0.1
			),
			'ResonanceModulation' -> (
				Spec: \bipolar.asSpec,
				LagTime: 0.1
			),
		]
	}

	*ugenGraphFunc {
		^{
			|
				in_In,
				in_FM,
				in_ResonanceModulation,
				out_Out,
				param_Frequency,
				param_Resonance,
				param_FM,
				param_ResonanceModulation
			|

			var sig_In = In.ar(in_In);
			var sig_FM = In.ar(in_FM);
			var sig_ResonanceModulation = In.ar(in_ResonanceModulation);

			var frequencySpec = \widefreq.asSpec;
			var resonanceSpec = \unipolar.asSpec;
			var frequency = frequencySpec.map(frequencySpec.unmap(param_Frequency) + (sig_FM * param_FM));
			var resonance = resonanceSpec.map(resonanceSpec.unmap(param_Resonance) + (sig_ResonanceModulation * param_ResonanceModulation));
			var sig = MoogLadder.ar(
				sig_In,
				frequency,
				resonance,
			);
			Out.ar(out_Out, sig);
		}
	}
}

// Status: tested
// Inspiration from A-140
// TODO: is it possible to implement Retrig(?)
RADSREnvelopeModule : RModule {
	*shortName { ^'ADSREnv' }

	*params {
		^[
			'Attack' -> (
				Spec: ControlSpec(0.1, 2000, 'exp', 0, 5, "ms"),
				LagTime: 0.1
			),
			'Decay' -> (
				Spec: ControlSpec(0.1, 8000, 'exp', 0, 200, "ms"),
				LagTime: 0.1
			),
			'Sustain' -> (
				Spec: ControlSpec(0, 1, 'lin', 0, 0.5, ""),
				LagTime: 0.1
			),
			'Release' -> (
				Spec: ControlSpec(0.1, 8000, 'exp', 0, 200, "ms"),
				LagTime: 0.1
			),
			'Gate' -> ControlSpec(0, 1, step: 1, default: 0)
//			'Curve' -> ControlSpec(-10, 10, 'lin', -4), TODO
		]
	}

	*ugenGraphFunc {
		^{
			|
				in_Gate,
				// TODO: Retrig
				out_Out,
				param_Attack,
				param_Decay,
				param_Sustain,
				param_Release,
				param_Gate/*,
				param_Curve TODO */
			|

			var sig_Gate = In.ar(in_Gate);
			var curve = -4; // TODO: default is -4, exp approximation -13.81523 !?
			Out.ar(
				out_Out,
				EnvGen.ar(
					Env.adsr(param_Attack/1000, param_Decay/1000, param_Sustain, param_Release/1000, curve: curve),
					((sig_Gate > 0) + (param_Gate > 0)) > 0,
					levelScale: 0.8 // TODO: ~ 8 V
				)
			);
		}
	}
}

// Status: partly tested
RSampleAndHoldModule : RModule {
	*shortName { ^'SampHold' }

	*ugenGraphFunc {
		^{
			|
				in_In,
				in_Trig,
				out_Out
			|

			Out.ar(
				out_Out,
				Latch.ar(In.ar(in_In), In.ar(in_Trig))
			);
		}
	}
}

// Status: partly tested
RRingModulatorModule : RModule {
	*shortName { ^'RingMod' }

	*ugenGraphFunc {
		^{
			|
				in_In,
				in_Carrier, // TODO: naming?
				out_Out
			|

			Out.ar(
				out_Out,
				In.ar(in_In) * In.ar(in_Carrier)
			);
		}
	}
}

// Status: tested, TODO: make a script
RNoiseModule : RModule {
	*shortName { ^'Noise' }

	*ugenGraphFunc {
		^{
			|
				out_Out
			|

			Out.ar(
				out_Out,
				WhiteNoise.ar
			);
		}
	}
}

// Status: partly tested. TODO: what modulation input range should be used?
RDelayModule : RModule {
	*shortName { ^'Delay' }

	*params {
		^[
			'DelayTime' -> (
				Spec: ControlSpec(0.1, 5000, 'exp', 0, 300, "ms"),
				LagTime: 0.25
			),
			'DelayTimeModulation' -> (
				Spec: \bipolar.asSpec,
				LagTime: 0.1
			),
		]
	}

	*ugenGraphFunc {
		^{
			|
				in_In,
				in_DelayTimeModulation,
				out_Out,
				param_DelayTime,
				param_DelayTimeModulation
			|

			var sig_In = In.ar(in_In);
			var sig_DelayTimeModulation = In.ar(in_DelayTimeModulation);

			var delayTimeSpec = ControlSpec(0.1, 5000, 'exp', 0, 300, "ms");

			var delayTimeMs = delayTimeSpec.map(
				delayTimeSpec.unmap(param_DelayTime) + (sig_DelayTimeModulation * param_DelayTimeModulation)
			);

			var delayed = DelayC.ar(sig_In, maxdelaytime: delayTimeSpec.maxval/1000, delaytime: delayTimeMs/1000); // TODO: ControlDur.ir minimum

			Out.ar(out_Out, delayed);
		}
	}
}

// Status: tested
RFreqGateModule : RModule {
	*shortName { ^'FreqGate' }

	*params {
		^[
			'Frequency' -> \freq.asSpec,
			'Gate' -> ControlSpec(0, 1, step: 1, default: 0)
		]
	}

	*ugenGraphFunc {
		^{
			|
				out_Frequency,
				out_Gate,
				out_Trig,
				param_Frequency,
				param_Gate
			|

			var sig_Gate = K2A.ar(param_Gate);
			var octs = ((param_Frequency.cpsoct-3)/10).clip(-0.5, 0.5); // 0.1 = 1 oct
			Out.ar(out_Frequency, K2A.ar(octs));
			Out.ar(out_Gate, sig_Gate);
			Out.ar(out_Gate, Trig.ar(sig_Gate));
		}
	}
}

// Status: partly tested. TODO: review modulation input mapping
RPitchShiftModule : RModule {
	*shortName { ^'PShift' }

	*params {
		^[
			'PitchRatio' -> (
				Spec: ControlSpec(0, 4, default: 1),
				LagTime: 0.1
			),
			'PitchDispersion' -> (
				Spec: ControlSpec(0, 4),
				LagTime: 0.1
			),
			'TimeDispersion' -> (
				Spec: ControlSpec(0, 1),
				LagTime: 0.1
			),
			'PitchRatioModulation' -> \bipolar.asSpec,
			'PitchDispersionModulation' -> \bipolar.asSpec,
			'TimeDispersionModulation' -> \bipolar.asSpec
		]
	}

	*ugenGraphFunc {
		^{
			|
				in_Left,
				in_Right,
				in_PitchRatioModulation,
				in_PitchDispersionModulation,
				in_TimeDispersionModulation,
				out_Left,
				out_Right,
				param_PitchRatio,
				param_PitchDispersion,
				param_TimeDispersion,
				param_PitchRatioModulation,
				param_PitchDispersionModulation,
				param_TimeDispersionModulation
			|

			var pitchRatioSpec = ControlSpec(0, 4, default: 1);
			var pitchDispersionSpec = ControlSpec(0, 4);
			var timeDispersionSpec = ControlSpec(0, 1);

			var sig_Left = In.ar(in_Left);
			var sig_Right = In.ar(in_Right);
			var sig_PitchRatioModulation = In.ar(in_PitchRatioModulation);
			var sig_PitchDispersionModulation = In.ar(in_PitchDispersionModulation);
			var sig_TimeDispersionModulation = In.ar(in_TimeDispersionModulation);

			var shifted = PitchShift.ar(
				[sig_Left, sig_Right],
				0.2,
				pitchRatioSpec.map(
					pitchRatioSpec.unmap(param_PitchRatio) +
					(sig_PitchRatioModulation * param_PitchRatioModulation)
				),
				pitchDispersionSpec.map(
					pitchDispersionSpec.unmap(param_PitchDispersion) +
					(sig_PitchDispersionModulation * param_PitchDispersionModulation)
				),
				timeDispersionSpec.map(
					timeDispersionSpec.unmap(param_TimeDispersion) +
					(sig_TimeDispersionModulation * param_TimeDispersionModulation)
				) / 5 // time dispersion cannot exceed windowSize (0.2)
			);

			Out.ar(out_Left, shifted[0]);
			Out.ar(out_Right, shifted[1]);
		}
	}
}

// Status: partly tested. TODO: review modulation input mapping
RFreqShiftModule : RModule {
	*shortName { ^'FShift' }

	*params {
		^[
			'Frequency' -> (
				Spec: ControlSpec(-2000, 2000, 'lin', 0, 0, "Hz"),
				LagTime: 0.1
			),
			'FM' -> (
				Spec: \bipolar.asSpec,
				LagTime: 0.1
			),
		]
	}

	*ugenGraphFunc {
		^{
			|
				in_Left,
				in_Right,
				in_FM,
				out_Left,
				out_Right,
				param_Frequency,
				param_FM
			|

			var frequencySpec = ControlSpec(-2000, 2000, 'lin', 0, 0, "Hz");

			var sig_Left = In.ar(in_Left);
			var sig_Right = In.ar(in_Right);
			var sig_FM = In.ar(in_FM);

			var shifted = FreqShift.ar(
				[sig_Left, sig_Right],
				frequencySpec.constrain(param_Frequency + (sig_FM * 2000 * param_FM))
			);

			Out.ar(out_Left, shifted[0]);
			Out.ar(out_Right, shifted[1]);
		}
	}
}

// Status: tested
RTestGenModule : RModule {
	*shortName { ^'TestGen' }

	*params {
		^[
			'Frequency' -> (
				Spec: \widefreq.asSpec,
				LagTime: 0 // TODO: all lag times have to be 0 if params with \db spec (such as param_Amplitude) are used (fixed by resorting to NamedControls in R?)
			),
			'Amplitude' -> \db.asSpec,
			'Wave' -> ControlSpec(0, 1, step: 1, default: 0)
		]
	}

	*ugenGraphFunc {
		^{
			|
				out_Out,
				param_Frequency,
				param_Amplitude,
				param_Wave
			|

			Out.ar(out_Out, SelectX.ar(param_Wave, [SinOsc.ar(param_Frequency), WhiteNoise.ar]) * param_Amplitude.dbamp);
		}
	}
}

// Status: tested
RMGainModule : RModule {
	*shortName { ^'MGain' }

	*params {
		^[
			'Gain' -> (
				Spec: \db.asSpec.copy.maxval_(12).default_(0),
				// TODO LagTime: 0.1
			),
			'Mute' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
		]
	}

	*ugenGraphFunc {
		^{
			|
				in_In,
				out_Out,
				param_Gain,
				param_Mute
			|

			var sig_In = In.ar(in_In);

			var gain = param_Gain.dbamp * Lag.kr(Select.kr(param_Mute, [1, 0], 0.01));

			Out.ar(out_Out, sig_In * gain);
		}
	}
}

// Status: tested
RSGainModule : RModule {
	*shortName { ^'SGain' }

	*params {
		^[
			'Gain' -> (
				Spec: \db.asSpec.copy.maxval_(12).default_(0),
				// TODO LagTime: 0.1
			),
			'Mute' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
		]
	}

	*ugenGraphFunc {
		^{
			|
				in_Left,
				in_Right,
				out_Left,
				out_Right,
				param_Gain,
				param_Mute
			|

			var sig_Left = In.ar(in_Left);
			var sig_Right = In.ar(in_Right);

			var gain = param_Gain.dbamp * Lag.kr(Select.kr(param_Mute, [1, 0], 0.01));

			Out.ar(out_Left, sig_Left * gain);
			Out.ar(out_Right, sig_Right * gain);
		}
	}
}

// Status: not tested
RQGainModule : RModule {
	*shortName { ^'QGain' }

	*params {
		^[
			'Gain' -> (
				Spec: \db.asSpec.copy.maxval_(12).default_(0),
				// TODO LagTime: 0.1
			),
			'Mute' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
		]
	}

	*ugenGraphFunc {
		^{
			|
				in_In1,
				in_In2,
				in_In3,
				in_In4,
				out_Out1,
				out_Out2,
				out_Out3,
				out_Out4,
				param_Gain,
				param_Mute
			|

			var sig_In1 = In.ar(in_In1);
			var sig_In2 = In.ar(in_In2);
			var sig_In3 = In.ar(in_In3);
			var sig_In4 = In.ar(in_In4);

			var gain = param_Gain.dbamp * Lag.kr(Select.kr(param_Mute, [1, 0], 0.01));

			Out.ar(out_Out1, sig_In1 * gain);
			Out.ar(out_Out2, sig_In2 * gain);
			Out.ar(out_Out3, sig_In3 * gain);
			Out.ar(out_Out4, sig_In4 * gain);
		}
	}
}

// Status: not tested
ROGainModule : RModule {
	*shortName { ^'OGain' }

	*params {
		^[
			'Gain' -> (
				Spec: \db.asSpec.copy.maxval_(12).default_(0),
				// TODO LagTime: 0.1
			),
			'Mute' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
		]
	}

	*ugenGraphFunc {
		^{
			|
				in_In1,
				in_In2,
				in_In3,
				in_In4,
				in_In5,
				in_In6,
				in_In7,
				in_In8,
				out_Out1,
				out_Out2,
				out_Out3,
				out_Out4,
				out_Out5,
				out_Out6,
				out_Out7,
				out_Out8,
				param_Gain,
				param_Mute
			|

			var sig_In1 = In.ar(in_In1);
			var sig_In2 = In.ar(in_In2);
			var sig_In3 = In.ar(in_In3);
			var sig_In4 = In.ar(in_In4);
			var sig_In5 = In.ar(in_In5);
			var sig_In6 = In.ar(in_In6);
			var sig_In7 = In.ar(in_In7);
			var sig_In8 = In.ar(in_In8);

			var gain = param_Gain.dbamp * Lag.kr(Select.kr(param_Mute, [1, 0], 0.01));

			Out.ar(out_Out1, sig_In1 * gain);
			Out.ar(out_Out2, sig_In2 * gain);
			Out.ar(out_Out3, sig_In3 * gain);
			Out.ar(out_Out4, sig_In4 * gain);
			Out.ar(out_Out5, sig_In5 * gain);
			Out.ar(out_Out6, sig_In6 * gain);
			Out.ar(out_Out7, sig_In7 * gain);
			Out.ar(out_Out8, sig_In8 * gain);
		}
	}
}

// Status: not tested
RCrossFaderModule : RModule {
	*shortName { ^'XFader' }

	*params {
		^[
			'Fade' -> \bipolar.asSpec, // TODO: remove need for .asSpec
			'TrimA' -> \db.asSpec.copy.maxval_(12),
			'TrimB' -> \db.asSpec.copy.maxval_(12),
			'Master' -> \db.asSpec.copy.maxval_(12)
		]
	}

	*ugenGraphFunc {
		^{
			|
				in_InALeft,
				in_InARight,
				in_InBLeft,
				in_InBRight,
				out_Left,
				out_Right,
				param_Fade,
				param_TrimA,
				param_TrimB,
				param_Master
			|

			var sig_InALeft = In.ar(in_InALeft);
			var sig_InARight = In.ar(in_InARight);
			var sig_InBLeft = In.ar(in_InBLeft);
			var sig_InBRight = In.ar(in_InBRight);

			var sig_inA = [sig_InALeft, sig_InARight] * param_TrimA.dbamp;
			var sig_inB = [sig_InBLeft, sig_InBRight] * param_TrimB.dbamp;
			var sig = XFade2.ar(sig_inA, sig_inB, param_Fade, param_Master.dbamp);
			Out.ar(out_Left, sig[0]);
			Out.ar(out_Right, sig[1]);
		}
	}
}

// Status: tested
R4x4MatrixModule : RModule {
	*shortName { ^'44Matrix' }

	*params {
		^[
			'FadeTime' -> ControlSpec(0, 1000, 'lin', 0, 5, "ms"),
			'Gate_1_1' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_1_2' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_1_3' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_1_4' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_2_1' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_2_2' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_2_3' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_2_4' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_3_1' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_3_2' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_3_3' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_3_4' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_4_1' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_4_2' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_4_3' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_4_4' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
		]
	}

	*ugenGraphFunc {
		^{
			|
				in_In1,
				in_In2,
				in_In3,
				in_In4,
				out_Out1,
				out_Out2,
				out_Out3,
				out_Out4,
				param_FadeTime,
				param_Gate_1_1,
				param_Gate_1_2,
				param_Gate_1_3,
				param_Gate_1_4,
				param_Gate_2_1,
				param_Gate_2_2,
				param_Gate_2_3,
				param_Gate_2_4,
				param_Gate_3_1,
				param_Gate_3_2,
				param_Gate_3_3,
				param_Gate_3_4,
				param_Gate_4_1,
				param_Gate_4_2,
				param_Gate_4_3,
				param_Gate_4_4
			|

			var sigs = [In.ar(in_In1), In.ar(in_In2), In.ar(in_In3), In.ar(in_In4)];

			Out.ar(
				out_Out1,
				(sigs[0] * Lag.kr(param_Gate_1_1, param_FadeTime/1000)) +
				(sigs[1] * Lag.kr(param_Gate_2_1, param_FadeTime/1000)) +
				(sigs[2] * Lag.kr(param_Gate_3_1, param_FadeTime/1000)) +
				(sigs[3] * Lag.kr(param_Gate_4_1, param_FadeTime/1000))
			);

			Out.ar(
				out_Out2,
				(sigs[0] * Lag.kr(param_Gate_1_2, param_FadeTime/1000)) +
				(sigs[1] * Lag.kr(param_Gate_2_2, param_FadeTime/1000)) +
				(sigs[2] * Lag.kr(param_Gate_3_2, param_FadeTime/1000)) +
				(sigs[3] * Lag.kr(param_Gate_4_2, param_FadeTime/1000))
			);

			Out.ar(
				out_Out3,
				(sigs[0] * Lag.kr(param_Gate_1_3, param_FadeTime/1000)) +
				(sigs[1] * Lag.kr(param_Gate_2_3, param_FadeTime/1000)) +
				(sigs[2] * Lag.kr(param_Gate_3_3, param_FadeTime/1000)) +
				(sigs[3] * Lag.kr(param_Gate_4_3, param_FadeTime/1000))
			);

			Out.ar(
				out_Out4,
				(sigs[0] * Lag.kr(param_Gate_1_4, param_FadeTime/1000)) +
				(sigs[1] * Lag.kr(param_Gate_2_4, param_FadeTime/1000)) +
				(sigs[2] * Lag.kr(param_Gate_3_4, param_FadeTime/1000)) +
				(sigs[3] * Lag.kr(param_Gate_4_4, param_FadeTime/1000))
			);
		}
	}
}

// Status: tested
R8x8MatrixModule : RModule {
	*shortName { ^'88Matrix' }

	*params {
		^[
			'FadeTime' -> ControlSpec(0, 1000, 'lin', 0, 5, "ms"),
			'Gate_1_1' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_1_2' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_1_3' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_1_4' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_1_5' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_1_6' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_1_7' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_1_8' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_2_1' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_2_2' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_2_3' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_2_4' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_2_5' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_2_6' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_2_7' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_2_8' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_3_1' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_3_2' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_3_3' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_3_4' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_3_5' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_3_6' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_3_7' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_3_8' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_4_1' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_4_2' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_4_3' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_4_4' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_4_5' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_4_6' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_4_7' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_4_8' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_5_1' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_5_2' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_5_3' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_5_4' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_5_5' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_5_6' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_5_7' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_5_8' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_6_1' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_6_2' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_6_3' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_6_4' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_6_5' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_6_6' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_6_7' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_6_8' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_7_1' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_7_2' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_7_3' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_7_4' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_7_5' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_7_6' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_7_7' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_7_8' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_8_1' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_8_2' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_8_3' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_8_4' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_8_5' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_8_6' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_8_7' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
			'Gate_8_8' -> ControlSpec(0, 1, 'lin', 1, 0, ""),
		]
	}

	*ugenGraphFunc {
		^{
			|
				in_In1,
				in_In2,
				in_In3,
				in_In4,
				in_In5,
				in_In6,
				in_In7,
				in_In8,
				out_Out1,
				out_Out2,
				out_Out3,
				out_Out4,
				out_Out5,
				out_Out6,
				out_Out7,
				out_Out8,
				param_FadeTime,
				param_Gate_1_1,
				param_Gate_1_2,
				param_Gate_1_3,
				param_Gate_1_4,
				param_Gate_1_5,
				param_Gate_1_6,
				param_Gate_1_7,
				param_Gate_1_8,
				param_Gate_2_1,
				param_Gate_2_2,
				param_Gate_2_3,
				param_Gate_2_4,
				param_Gate_2_5,
				param_Gate_2_6,
				param_Gate_2_7,
				param_Gate_2_8,
				param_Gate_3_1,
				param_Gate_3_2,
				param_Gate_3_3,
				param_Gate_3_4,
				param_Gate_3_5,
				param_Gate_3_6,
				param_Gate_3_7,
				param_Gate_3_8,
				param_Gate_4_1,
				param_Gate_4_2,
				param_Gate_4_3,
				param_Gate_4_4,
				param_Gate_4_5,
				param_Gate_4_6,
				param_Gate_4_7,
				param_Gate_4_8,
				param_Gate_5_1,
				param_Gate_5_2,
				param_Gate_5_3,
				param_Gate_5_4,
				param_Gate_5_5,
				param_Gate_5_6,
				param_Gate_5_7,
				param_Gate_5_8,
				param_Gate_6_1,
				param_Gate_6_2,
				param_Gate_6_3,
				param_Gate_6_4,
				param_Gate_6_5,
				param_Gate_6_6,
				param_Gate_6_7,
				param_Gate_6_8,
				param_Gate_7_1,
				param_Gate_7_2,
				param_Gate_7_3,
				param_Gate_7_4,
				param_Gate_7_5,
				param_Gate_7_6,
				param_Gate_7_7,
				param_Gate_7_8,
				param_Gate_8_1,
				param_Gate_8_2,
				param_Gate_8_3,
				param_Gate_8_4,
				param_Gate_8_5,
				param_Gate_8_6,
				param_Gate_8_7,
				param_Gate_8_8
			|

			var sigs = [In.ar(in_In1), In.ar(in_In2), In.ar(in_In3), In.ar(in_In4), In.ar(in_In5), In.ar(in_In6), In.ar(in_In7), In.ar(in_In8)];

			Out.ar(
				out_Out1,
				(sigs[0] * Lag.kr(param_Gate_1_1, param_FadeTime/1000)) +
				(sigs[1] * Lag.kr(param_Gate_2_1, param_FadeTime/1000)) +
				(sigs[2] * Lag.kr(param_Gate_3_1, param_FadeTime/1000)) +
				(sigs[3] * Lag.kr(param_Gate_4_1, param_FadeTime/1000)) +
				(sigs[4] * Lag.kr(param_Gate_5_1, param_FadeTime/1000)) +
				(sigs[5] * Lag.kr(param_Gate_6_1, param_FadeTime/1000)) +
				(sigs[6] * Lag.kr(param_Gate_7_1, param_FadeTime/1000)) +
				(sigs[7] * Lag.kr(param_Gate_8_1, param_FadeTime/1000))
			);

			Out.ar(
				out_Out2,
				(sigs[0] * Lag.kr(param_Gate_1_2, param_FadeTime/1000)) +
				(sigs[1] * Lag.kr(param_Gate_2_2, param_FadeTime/1000)) +
				(sigs[2] * Lag.kr(param_Gate_3_2, param_FadeTime/1000)) +
				(sigs[3] * Lag.kr(param_Gate_4_2, param_FadeTime/1000)) +
				(sigs[4] * Lag.kr(param_Gate_5_2, param_FadeTime/1000)) +
				(sigs[5] * Lag.kr(param_Gate_6_2, param_FadeTime/1000)) +
				(sigs[6] * Lag.kr(param_Gate_7_2, param_FadeTime/1000)) +
				(sigs[7] * Lag.kr(param_Gate_8_2, param_FadeTime/1000))
			);

			Out.ar(
				out_Out3,
				(sigs[0] * Lag.kr(param_Gate_1_3, param_FadeTime/1000)) +
				(sigs[1] * Lag.kr(param_Gate_2_3, param_FadeTime/1000)) +
				(sigs[2] * Lag.kr(param_Gate_3_3, param_FadeTime/1000)) +
				(sigs[3] * Lag.kr(param_Gate_4_3, param_FadeTime/1000)) +
				(sigs[4] * Lag.kr(param_Gate_5_3, param_FadeTime/1000)) +
				(sigs[5] * Lag.kr(param_Gate_6_3, param_FadeTime/1000)) +
				(sigs[6] * Lag.kr(param_Gate_7_3, param_FadeTime/1000)) +
				(sigs[7] * Lag.kr(param_Gate_8_3, param_FadeTime/1000))
			);

			Out.ar(
				out_Out4,
				(sigs[0] * Lag.kr(param_Gate_1_4, param_FadeTime/1000)) +
				(sigs[1] * Lag.kr(param_Gate_2_4, param_FadeTime/1000)) +
				(sigs[2] * Lag.kr(param_Gate_3_4, param_FadeTime/1000)) +
				(sigs[3] * Lag.kr(param_Gate_4_4, param_FadeTime/1000)) +
				(sigs[4] * Lag.kr(param_Gate_5_4, param_FadeTime/1000)) +
				(sigs[5] * Lag.kr(param_Gate_6_4, param_FadeTime/1000)) +
				(sigs[6] * Lag.kr(param_Gate_7_4, param_FadeTime/1000)) +
				(sigs[7] * Lag.kr(param_Gate_8_4, param_FadeTime/1000))
			);

			Out.ar(
				out_Out5,
				(sigs[0] * Lag.kr(param_Gate_1_5, param_FadeTime/1000)) +
				(sigs[1] * Lag.kr(param_Gate_2_5, param_FadeTime/1000)) +
				(sigs[2] * Lag.kr(param_Gate_3_5, param_FadeTime/1000)) +
				(sigs[3] * Lag.kr(param_Gate_4_5, param_FadeTime/1000)) +
				(sigs[4] * Lag.kr(param_Gate_5_5, param_FadeTime/1000)) +
				(sigs[5] * Lag.kr(param_Gate_6_5, param_FadeTime/1000)) +
				(sigs[6] * Lag.kr(param_Gate_7_5, param_FadeTime/1000)) +
				(sigs[7] * Lag.kr(param_Gate_8_5, param_FadeTime/1000))
			);

			Out.ar(
				out_Out6,
				(sigs[0] * Lag.kr(param_Gate_1_6, param_FadeTime/1000)) +
				(sigs[1] * Lag.kr(param_Gate_2_6, param_FadeTime/1000)) +
				(sigs[2] * Lag.kr(param_Gate_3_6, param_FadeTime/1000)) +
				(sigs[3] * Lag.kr(param_Gate_4_6, param_FadeTime/1000)) +
				(sigs[4] * Lag.kr(param_Gate_5_6, param_FadeTime/1000)) +
				(sigs[5] * Lag.kr(param_Gate_6_6, param_FadeTime/1000)) +
				(sigs[6] * Lag.kr(param_Gate_7_6, param_FadeTime/1000)) +
				(sigs[7] * Lag.kr(param_Gate_8_6, param_FadeTime/1000))
			);

			Out.ar(
				out_Out7,
				(sigs[0] * Lag.kr(param_Gate_1_7, param_FadeTime/1000)) +
				(sigs[1] * Lag.kr(param_Gate_2_7, param_FadeTime/1000)) +
				(sigs[2] * Lag.kr(param_Gate_3_7, param_FadeTime/1000)) +
				(sigs[3] * Lag.kr(param_Gate_4_7, param_FadeTime/1000)) +
				(sigs[4] * Lag.kr(param_Gate_5_7, param_FadeTime/1000)) +
				(sigs[5] * Lag.kr(param_Gate_6_7, param_FadeTime/1000)) +
				(sigs[6] * Lag.kr(param_Gate_7_7, param_FadeTime/1000)) +
				(sigs[7] * Lag.kr(param_Gate_8_7, param_FadeTime/1000))
			);

			Out.ar(
				out_Out8,
				(sigs[0] * Lag.kr(param_Gate_1_8, param_FadeTime/1000)) +
				(sigs[1] * Lag.kr(param_Gate_2_8, param_FadeTime/1000)) +
				(sigs[2] * Lag.kr(param_Gate_3_8, param_FadeTime/1000)) +
				(sigs[3] * Lag.kr(param_Gate_4_8, param_FadeTime/1000)) +
				(sigs[4] * Lag.kr(param_Gate_5_8, param_FadeTime/1000)) +
				(sigs[5] * Lag.kr(param_Gate_6_8, param_FadeTime/1000)) +
				(sigs[6] * Lag.kr(param_Gate_7_8, param_FadeTime/1000)) +
				(sigs[7] * Lag.kr(param_Gate_8_8, param_FadeTime/1000))
			);

		}
	}
}

// Status: not tested
RFMVoiceModule : RModule {
	classvar numOscs = 3;

	*shortName { ^'FMVoice' }

	*params {
		var params = [
			'Freq' -> \freq.asSpec,
			'Timbre' -> ControlSpec(0, 5, 'lin', nil, 1, "")
		];

		numOscs.do { |oscnum|
			params = params.addAll(
				[
					"Osc%Gain".format(oscnum+1) -> \amp.asSpec,
					"Osc%Partial".format(oscnum+1) -> ControlSpec(0.5, 12, 'lin', 0.5, 1, ""),
					"Osc%Fixed".format(oscnum+1) -> ControlSpec(0, 1, 'lin', 1, 0, ""),
					"Osc%Fixedfreq".format(oscnum+1) -> \widefreq.asSpec,
					"Osc%Index".format(oscnum+1) -> ControlSpec(0, 24, 'lin', 0, 3, ""),
					"Osc%Outlevel".format(oscnum+1) -> \amp.asSpec,
					"Mod_To_Osc%Freq".format(oscnum+1) -> \bipolar.asSpec,
					"Mod_To_Osc%Gain".format(oscnum+1) -> \bipolar.asSpec,
				]
			);

			numOscs.do { |dest|
				params = params.add(
					"Osc%_To_Osc%Freq".format(oscnum+1, dest+1) -> \amp.asSpec
				);
			};
		};

		^params.collect { |assoc|
			assoc.key.asSymbol -> assoc.value
		};
	}

	*ugenGraphFunc {
		^{
			|
				in_Modulation,
				out_Out,
				param_Freq,
				param_Timbre,
				param_Osc1Gain,
				param_Osc1Partial,
				param_Osc1Fixed,
				param_Osc1Fixedfreq,
				param_Osc1Index,
				param_Osc1Outlevel,
				param_Osc1_To_Osc1Freq,
				param_Osc1_To_Osc2Freq,
				param_Osc1_To_Osc3Freq,
				param_Osc2Gain,
				param_Osc2Partial,
				param_Osc2Fixed,
				param_Osc2Fixedfreq,
				param_Osc2Index,
				param_Osc2Outlevel,
				param_Osc2_To_Osc1Freq,
				param_Osc2_To_Osc2Freq,
				param_Osc2_To_Osc3Freq,
				param_Osc3Gain,
				param_Osc3Partial,
				param_Osc3Fixed,
				param_Osc3Fixedfreq,
				param_Osc3Index,
				param_Osc3Outlevel,
				param_Osc3_To_Osc3Freq,
				param_Osc3_To_Osc2Freq,
				param_Osc3_To_Osc1Freq,
				param_Mod_To_Osc1Gain,
				param_Mod_To_Osc2Gain,
				param_Mod_To_Osc3Gain,
				param_Mod_To_Osc1Freq,
				param_Mod_To_Osc2Freq,
				param_Mod_To_Osc3Freq
			|

			var sig;

			var osc1, osc2, osc3;
			var osc1freq, osc2freq, osc3freq;
			var osc1freqbasemod, osc2freqbasemod, osc3freqbasemod;
			var oscfeedback = LocalIn.ar(3);
			var sig_Modulation = In.ar(in_Modulation);

			osc1freq = Select.kr(param_Osc1Fixed, [param_Freq*param_Osc1Partial, param_Osc1Fixedfreq]);
			osc2freq = Select.kr(param_Osc2Fixed, [param_Freq*param_Osc2Partial, param_Osc2Fixedfreq]);
			osc3freq = Select.kr(param_Osc3Fixed, [param_Freq*param_Osc3Partial, param_Osc3Fixedfreq]);

			osc1freqbasemod = param_Osc1Index * osc1freq * param_Timbre;
			osc2freqbasemod = param_Osc2Index * osc2freq * param_Timbre;
			osc3freqbasemod = param_Osc3Index * osc3freq * param_Timbre;

			osc1 = SinOsc.ar(
				osc1freq
					+ (osc1freqbasemod * oscfeedback[0] * param_Osc1_To_Osc1Freq)
					+ (osc1freqbasemod * oscfeedback[1] * param_Osc2_To_Osc1Freq)
					+ (osc1freqbasemod * oscfeedback[2] * param_Osc3_To_Osc1Freq)
					+ (osc1freqbasemod * sig_Modulation * param_Mod_To_Osc1Freq)
			) * (param_Osc1Gain + (param_Mod_To_Osc1Gain * sig_Modulation));

			osc2 = SinOsc.ar(
				osc2freq
					+ (osc2freqbasemod * osc1 * param_Osc1_To_Osc2Freq)
					+ (osc2freqbasemod * oscfeedback[1] * param_Osc2_To_Osc2Freq)
					+ (osc2freqbasemod * oscfeedback[2] * param_Osc3_To_Osc2Freq)
					+ (osc2freqbasemod * sig_Modulation * param_Mod_To_Osc2Freq)
			) * (param_Osc2Gain + (param_Mod_To_Osc2Gain * sig_Modulation));

			osc3 = SinOsc.ar(
				osc3freq
					+ (osc3freqbasemod * osc1 * param_Osc1_To_Osc3Freq)
					+ (osc3freqbasemod * osc2 * param_Osc2_To_Osc3Freq)
					+ (osc3freqbasemod * oscfeedback[2] * param_Osc3_To_Osc3Freq)
					+ (osc3freqbasemod * sig_Modulation * param_Mod_To_Osc3Freq)
			) * (param_Osc3Gain + (param_Mod_To_Osc3Gain * sig_Modulation));

			sig = (osc1 * param_Osc1Outlevel) + (osc2 * param_Osc2Outlevel) + (osc3 * param_Osc3Outlevel);

			LocalOut.ar([osc1, osc2, osc3]);
			Out.ar(out_Out, sig);
		}
	}
}
