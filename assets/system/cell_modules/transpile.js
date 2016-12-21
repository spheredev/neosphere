'use strict';
module.exports =
{
};

const CoffeeScriptTool = new Tool(buildCoffeeScript);

function buildCoffeeScript(outName, inNames, options)
{
	CoffeeScriptTool.build(outName, inNames, options);
}
