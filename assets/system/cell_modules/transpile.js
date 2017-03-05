/**
 *  Cell Standard Library 'transpile' module
 *  (c) 2017 Fat Cerberus
**/

module.exports = transpile;
const from  = require('from');
const ts    = require('#/typescript');

function transpile(dirName, sources)
{
	return stageTargets(dirName, sources, 'auto');
}

transpile.v1 = transpileScripts;
function transpileScripts(dirName, sources)
{
	return stageTargets(dirName, sources, scriptTool);
}

transpile.v2 = transpileModules;
function transpileModules(dirName, sources)
{
	return stageTargets(dirName, sources, moduleTool);
}

var moduleTool = makeTranspilerTool(2.0);
var scriptTool = makeTranspilerTool(1.0);

function makeTranspilerTool(apiVersion)
{
	return new Tool(function(outFileName, inFileNames) {
		var fileContent = FS.readFile(inFileNames[0]);
		var input = new TextDecoder().decode(fileContent);
		var output = ts.transpileModule(input, {
			fileName: inFileNames[0],
			reportDiagnostics: true,
			compilerOptions: {
				module: ts.ModuleKind.CommonJS,
				moduleResolution: ts.ModuleResolutionKind.NodeJs,
				noImplicitUseStrict: apiVersion <= 1.0,
				newLine: ts.NewLineKind.LineFeed,
				target: ts.ScriptTarget.ES5,
			},
		});
		FS.writeFile(outFileName, new TextEncoder().encode(output.outputText));
		from(output.diagnostics)
			.where(function(v) { return typeof v.messageText === 'string'; })
			.each(function(line)
		{
			var message = "TS" + line.code + ": " + line.messageText;
			if (line.category === ts.DiagnosticCategory.Error)
				error(message);
			else
				warn(message);
		});
	}, "transpiling");
}

function stageTargets(dirName, sources, tool)
{
	var targets = [];
	FS.createDirectory(dirName);
	for (var i = 0; i < sources.length; ++i) {
		var fileName = FS.resolve(dirName + '/' + sources[i].name);
		var currentTool = tool !== 'auto' ? tool
			: fileName.endsWith('.mjs') ? moduleTool
			: fileName.endsWith('.ts') ? moduleTool
			: scriptTool;
		var target = currentTool.stage(fileName, [ sources[i] ], {
			name: sources[i].name,
		});
		targets[targets.length] = target;
	}
	return targets;
}
