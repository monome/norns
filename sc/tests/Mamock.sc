Mamock {
	classvar
		<>trace = true,
		matronRecvPort = 8888,
		<croneAddr,
		default
	;

	var <oscFuncs;

	*initClass {
	}

	*default {
		^if (default.notNil) {
			default
		} {
			default = this.new
		}
	}

	*croneLoadEngine { |name|
		this.default.croneLoadEngine(name);
	}

	*croneReportEngines {
		this.default.croneReportEngines;
	}

	*croneReportCommands {
		this.default.croneReportCommands;
	}

	*croneReportParameters {
		this.default.croneReportParameters;
	}

	*croneReportPolls {
		this.default.croneReportPolls;
	}

	*croneCommand { |cmd ... args|
		this.default.croneCommand(cmd, *args);
	}

	*croneParameter { |parameter ... args|
		this.default.croneParameter(parameter, *args);
	}

	*croneMsg { |...args|
		this.default.croneMsg(*args);
	}

	*new { ^super.new.initmmmmmmm }

	initmmmmmmm {
		croneAddr = NetAddr.localAddr;

		thisProcess.openUDPPort(matronRecvPort); // enable matron sc port

		["engines", "commands", "parameters", "polls"].do { |type|
			["start", "entry", "end"].do { |suffix|
				var path = "/report/" ++ type ++ "/" ++ suffix;
				oscFuncs = oscFuncs.add(
					OSCFunc(
						{
							|msg, time, addr, recvPort|
							this.traceComm(\mmock_received, [msg, time, addr, recvPort]);
						},
						path,
						recvPort: matronRecvPort
					)
				);
			};
		}

	}

	croneLoadEngine { |name|
		this.croneMsg("/engine/load/name", name);
	}

	croneReportEngines {
		this.croneMsg("/report/engines");
	}

	croneReportCommands {
		this.croneMsg("/report/commands");
	}

	croneReportPolls {
		this.croneMsg("/report/polls");
	}

	croneReportParameters {
		this.croneMsg("/report/parameters");
	}

	croneCommand { |cmd ... args|
		var msg = ["/command/" ++ cmd.asString].addAll(args);
		this.traceComm(\mmock_sent, msg);
		this.croneMsg(*msg);
	}

	croneParameter { |cmd ... args|
		var msg = ["/parameter/" ++ cmd.asString].addAll(args);
		this.traceComm(\mmock_sent, msg);
		this.croneMsg(*msg);
	}

	croneMsg { |...args|
		croneAddr.sendMsg(*args);
	}

	traceComm { |...msg|
		if (trace) {
			msg.debug(\mmock_sent);
		}
	}
}
