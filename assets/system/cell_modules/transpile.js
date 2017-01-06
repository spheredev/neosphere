'use strict';
module.exports = main();

const ts = require('./lib/typescript');

function main()
{
	var TS = new Tool(compileTS, "transpile");
	return {
		TS
	};
}

function compileTS(outName, inNames, options)
{
	if (inNames.length == 0)
		throw new RangeError("one or more sources required");
}
