/**
 *  Cell Standard Library 'transpile' module
 *  (c) 2017 Fat Cerberus
**/

'use strict';
module.exports = transpile;

const Babel = require('./lib/Babel');

var moduleTool = new Tool(TOOLFUNC('commonjs'), "transpiling");
var scriptTool = new Tool(TOOLFUNC(false), "transpiling");

function TOOLFUNC(moduleType)
{
	return function(outFileName, inFileNames)
	{
		var fileContent = FS.readFile(inFileNames[0]);
		var input = new TextDecoder().decode(fileContent);
		var output = Babel.transform(input, {
			presets: [
				[ 'latest', { es2015: {
					modules: moduleType,
					spec:    true,
				} } ],
			],
			comments:    false,
			retainLines: true,
		});
		FS.writeFile(outFileName, new TextEncoder().encode(output.code));
	}
}

function doTranspile(dirName, sources, tool)
{
	var targets = [];
	FS.createDirectory(dirName);
	for (var i = 0; i < sources.length; ++i) {
		var fileName = FS.resolve(dirName + '/' + sources[i].name);
		var target = tool.stage(fileName, [ sources[i] ], {
			name: sources[i].name,
		});
		targets[targets.length] = target;
	}
	return targets;
}

transpile.v2 = transpile;
function transpile(dirName, sources)
{
	return doTranspile(dirName, sources, moduleTool);
}

transpile.v1 = transpileAsScripts;
function transpileAsScripts(dirName, sources)
{
	return doTranspile(dirName, sources, scriptTool);
}
