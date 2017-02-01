/**
 *  Cell Standard Library 'transpile' module
 *  (c) 2017 Fat Cerberus
**/

import * as Babel from './lib/babel-core';

export default
function transpile(dirName, sources)
{
	return stageTargets(dirName, sources, moduleTool);
}

transpile.v1 = transpileScripts;
function transpileScripts(dirName, sources)
{
	return stageTargets(dirName, sources, scriptTool);
}

var moduleTool = makeTranspilerTool(2.0);
var scriptTool = makeTranspilerTool(1.0);

function makeTranspilerTool(apiVersion)
{
	return new Tool((outFileName, inFileNames) => {
		var moduleType = apiVersion >= 2.0 ? 'commonjs' : false;
		var sourceType = apiVersion >= 2.0 ? 'module' : 'script';
		var fileContent = FS.readFile(inFileNames[0]);
		var input = new TextDecoder().decode(fileContent);
		var output = Babel.transform(input, {
			sourceType,
			comments: false,
			retainLines: true,
			presets: [
				[ 'latest', { es2015: { modules: moduleType } } ]
			]
		});
		FS.writeFile(outFileName, new TextEncoder().encode(output.code));
	}, "transpiling");
}

function stageTargets(dirName, sources, tool)
{
	var targets = [];
	FS.createDirectory(dirName);
	for (const source of sources) {
		var fileName = FS.resolve(dirName + '/' + source.name);
		var target = tool.stage(fileName, [ source ], {
			name: source.name,
		});
		targets[targets.length] = target;
	}
	return targets;
}
