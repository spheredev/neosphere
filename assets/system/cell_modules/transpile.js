'use strict';
module.exports = main();

const coffee = require('./lib/coffee-script.lib');

function main()
{
	var coffeeTool = new Tool(buildCoffeeScript);
	return coffeeTool;
}

function buildCoffeeScript(outName, inNames, options)
{
	if (inNames.length != 1)
		throw new RangeError("CoffeeScript requires exactly one source");

	var file = FS.open(inNames[0], 'rb');
	var source = new TextDecoder().decode(file.read());
	file.close();

	var js = coffee.compile(source, { filename: inNames[0] });
	file = FS.open(outName, 'wb');
	file.write(new TextEncoder().encode(js));
	file.close();
}
