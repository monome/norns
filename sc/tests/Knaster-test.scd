Mamock.croneReportEngines;
Mamock.croneLoadEngine("Knaster").
Mamock.croneReportParameters.
Mamock.croneReportCommands.
Mamock.croneCommand("start");

(
// UI
var dbspec = \db.asSpec;

a=Window.new;
c=Slider.new(a, Rect(0, 0, 40, 200));
c.value = dbspec.unmap(dbspec.default).debug;
c.action = { |slider|
	Mamock.croneParameter("db", \db.asSpec.map(slider.value));
};
a.front;
)

Mamock.croneParameter("db", -20);
Mamock.croneParameter("db", -10);
Mamock.croneCommand("stop");
