/**
 *  Cell Standard Library 'transpile' module
 *  (c) 2017 Fat Cerberus
**/

module.exports = transpile;
const Babel = require('#/babel-core');

transpile.v2 = transpile;
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
	return new Tool(function(outFileName, inFileNames) {
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
	for (var i = 0; i < sources.length; ++i) {
		var fileName = FS.resolve(dirName + '/' + sources[i].name);
		var target = tool.stage(fileName, [ sources[i] ], {
			name: sources[i].name,
		});
		targets[targets.length] = target;
	}
	return targets;
}
