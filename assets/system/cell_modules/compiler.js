/**
 *  Cell Standard Library 'compiler' module
 *  (c) 2017 Fat Cerberus
**/

'use strict';
module.exports =
{
	__esModule: true,  // Babel interop
	transpile: transpile,
};

const from = require('from');
const ts   = require('#/typescript');

const ModuleTool = makeTranspileTool(2.0);
const ScriptTool = makeTranspileTool(1.0);

function transpile(dirName, sources)
{
	return stageTranspileJob(dirName, sources);
}

function makeTranspileTool(apiVersion)
{
	return new Tool(function(outFileName, inFileNames) {
		var fileContent = FS.readFile(inFileNames[0]);
		var input = new TextDecoder().decode(fileContent);
		var output = ts.transpileModule(input, {
			fileName: inFileNames[0],
			reportDiagnostics: true,
			compilerOptions: {
				target: ts.ScriptTarget.ES5,
				module: ts.ModuleKind.CommonJS,
				allowJs: true,
				downlevelIteration: true,
				newLine: ts.NewLineKind.LineFeed,
				noImplicitUseStrict: apiVersion <= 1.0,
				sourceMap: true,
			},
		});
		if (from(output.diagnostics).all(function(v) { return v.category !== ts.DiagnosticCategory.Error; })) {
			FS.writeFile(outFileName, new TextEncoder().encode(output.outputText));
			FS.writeFile(outFileName + '.map', new TextEncoder().encode(output.sourceMapText));
        }
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
	}, "compiling");
}

function stageTranspileJob(dirName, sources)
{
	var targets = [];
	FS.createDirectory(dirName);
	for (var i = 0; i < sources.length; ++i) {
		var fileName = FS.resolve(dirName + '/' + sources[i].name);
		var tool = fileName.endsWith('.mjs') ? ModuleTool : ScriptTool;
		var target = tool.stage(fileName, [ sources[i] ], {
			name: sources[i].name,
		});
		targets[targets.length] = target;
	}
	return targets;
}
