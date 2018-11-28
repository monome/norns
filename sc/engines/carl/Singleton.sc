//Singleton from Singleton SuperCollider Quark by Scott Carver

Singleton {
	classvar all, <>know=false, creatingNew=false;
	var <>name;

	*initClass {
		all = IdentityDictionary();
	}

	*default {
		^\default
	}

	*all {
		^all[this] ?? IdentityDictionary()
	}

	*new {
		arg name ...settings;
		var sing, classAll, created=false;
		name = name ?? this.default;

		classAll = all.atFail(this, {
			all[this] = IdentityDictionary();
			all[this];
		});

		sing = classAll.atFail(name, {
			var newSingleton = this.createNew();
			created = true;
			newSingleton.name = name;
			classAll[name] = newSingleton;
			newSingleton.init(name);
			newSingleton;
		});

		if ((settings.notNil && settings.notEmpty) || created) {
			sing.set(*settings)
		};
		if (created) { { this.changed(\added, sing) }.defer(0) };
		^sing;
	}

	*createNew {
		arg ...args;
		^super.new(*args);
	}

	*doesNotUnderstand { arg selector ... args;
		var item;

		if (know && creatingNew.not) {
			creatingNew = true;		// avoid reentrancy
			protect {
				if (selector.isSetter) {
					selector = selector.asString;
					selector = selector[0..(selector.size - 2)].asSymbol;
					item = this.new(selector, *args);
				} {
					item = this.new(selector);
				}
			} {
				creatingNew = false;
			};

			^item;
		} {
			^this.superPerformList(\doesNotUnderstand, selector, args);
		}
	}

	init {}

	set {
		// Override this to receive 'settings' parameter from Singleton.new(name, settings)
	}

	*clear {
		|sing|
		var dict = all[this];
		if (dict.notNil) {
			var key = dict.findKeyForValue(sing);
			if (key.notNil) {
				dict[key] = nil;
				this.changed(\removed, sing);
			}
		}
	}

	clear {
		this.class.clear(this);
	}
}