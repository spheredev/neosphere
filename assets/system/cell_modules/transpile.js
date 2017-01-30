/**
 *  Cell Standard Library 'transpile' module
 *  (c) 2017 Fat Cerberus
**/
'use strict';

const Babel = require('./lib/babel-core');

exports = module.exports = transpileModules;
exports.v2 = transpileModules;
function transpileModules(dirName, sources)
{
	return stageTargets(dirName, sources, moduleTool);
}

exports.v1 = transpileScripts;
function transpileScripts(dirName, sources)
{
	return stageTargets(dirName, sources, scriptTool);
}

var moduleTool = makeTranspilerTool(2);
var scriptTool = makeTranspilerTool(1);

function makeTranspilerTool(apiVersion)
{
	return new Tool(function(outFileName, inFileNames) {
		var fileContent = FS.readFile(inFileNames[0]);
		var input = new TextDecoder().decode(fileContent);
		var moduleType = apiVersion >= 2 ? 'commonjs' : false;
		var sourceType = apiVersion >= 2 ? 'module' : 'script';
		var output = Babel.transform(input, {
			sourceType: sourceType,
			comments: false,
			retainLines: true,
			presets: [
				[ 'latest', { es2015: { modules: moduleType } } ],
			],
		});
		FS.writeFile(outFileName, new TextEncoder().encode(output.code));
	}, "transpiling");
}

function stageTargets(dirName, sources, tool)
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
