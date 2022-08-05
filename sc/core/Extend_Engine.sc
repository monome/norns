// needed extension to Engine class file from main library

+ ContiguousBlockAllocator {
	totalUsed {
		var sum = 0;
		array.do({ arg block;
			if (block.notNil, {
				if (block.used, { sum = sum + block.size });
			});
		});
		^sum
	}

	totalFree {
		var sum = 0;
		array.do({ arg block;
			if (block.notNil, {
				if (block.used.not, { sum = sum + block.size });
			});
		});
		^sum
	}
}
