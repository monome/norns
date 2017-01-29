// the Crone, a singleton class
// it receives OSC from *matron* and manages the current CroneEngine
Crone {
	classvar engine; // current CroneEngine instance
	classvar oscfunc;
	classvar remote_addr;

	*initClass {
		StartUp.add { // defer until after sclang init

			postln("Crone class startup");

			Server.default.boot;

			// FIXME: hardcoded remote address for now
			remote_addr =NetAddr("127.0.0.1", 8888);

			// FIXME: osc patterns should be customized by current engine
			// this is dependent on adding support to the C side
			oscfunc = (
				'/engine/kill':OSCFunc.new({
					if(engine.notNil, { engine.kill; });
				}, '/engine/kill'),

				'/engine/request/report':OSCFunc.new({
					arg msg, time, addr, recvPort;
					[msg, time, addr, recvPort].postln;
					this.reportEngines;
				}, '/engine/request/report'),

				'/engine/load/name':OSCFunc.new({
					arg msg, time, addr, recvPort;
					[msg,time,addr,recvPort].postln;
					this.setEngine("Crone_" ++ msg[1]);
				}, '/engine/load/name'),

				'/buffer/load/name':OSCFunc.new({
					arg msg, time, addr, recvPort;
					engine.loadBufferByName(msg[1], msg[2]);
				}, '/buffer/load/name'),

				'/param/set/name':OSCFunc.new({
					arg msg, time, addr, recvPort;
					engine.setParamByName(msg[1], msg[2]);
				}, '/param/set/name'),

				'/param/set/idx':OSCFunc.new({
					arg msg, time, addr, recvPort;
					engine.setParamByIndex(msg[1], msg[2]);
				}, '/param/set/idx')
			);
		}
	}


	*setEngine { arg name;
		var class;
		class = CroneEngine.subclasses.select({ arg n; n.asString == name.asString })[0];
		postln("setEngine class: " ++ class);
		if(class.notNil, {
			if(engine.notNil, {
				engine.kill;
			});
			engine = class.new(Server.default);
		});
	}

	*reportEngines{
		var names = CroneEngine.subclasses.collect({ arg n;
			n.asString.split($_)[1]
		});
		postln("sending CroneEngine subclass list: ");
		postln(names);
		remote_addr.sendMsg("/engine/report/start", names.size);
		names.do({ arg name, i;
			remote_addr.sendMsg("/engine/report/name", i, name);

		});
		remote_addr.sendMsg("/engine/report/end");
	}

	*reportBuffers{
		var names = engine.buffers;
		remote_addr.sendMsg("/buffer/report/start", names.size);
		names.do({ arg name, i; remote_addr.sendMsg("/buffer/report/name", i, name); });
		remote_addr.sendMsg("/buffer/report/end");
	}

	*reportParams {
		var names = engine.paramNames;
		remote_addr.sendMsg("/param/report/start", names.size);
		names.do({ arg name, i; remote_addr.sendMsg("/param/report/name", i, name); });
		remote_addr.sendMsg("/param/report/end");
	}
}