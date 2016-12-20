'use strict';
module.exports =
{
	coffeescript: coffeescript,
};

const cs = require('./lib/coffeescript');
const ts = require('./lib/typescript');

const CoffeeTool = new Tool(function);

function coffeescript(out, in)
{
	var target = new Target(CoffeeTool, out, in);
}
