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

const from  = require('from');
const ts    = require('#/typescript');

const ModuleTool = makeTranspileTool(2.0);
const ScriptTool = makeTranspileTool(1.0);

function transpile(dirName, sources)
{
	return stageTranspileJob(dirName, sources);
}

function makeTranspileTool(apiVersion)
{
	return new Tool(function(outFileName, inFileNames) {
		var input = FS.readFile(inFileNames[0]);
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
				inlineSourceMap: true,
				inlineSources: true,
			},
		});
		if (from(output.diagnostics).all(function(v) { return v.category !== ts.DiagnosticCategory.Error; }))
			FS.writeFile(outFileName, output.outputText);
		from(output.diagnostics).each(function(diag) {
			var message = ts.flattenDiagnosticMessageText(diag.messageText, '\n');
			var errorCode = "TS" + diag.code;
			if (diag.file !== undefined) {
				var lineNumber = 1 + ts.getLineAndCharacterOfPosition(diag.file, diag.start).line;
				message = errorCode + " [" + diag.file.fileName + ":" + lineNumber + "]: " + message;
			}
			else {
				message = errorCode + ": " + message;
			}
			switch (diag.category) {
				case ts.DiagnosticCategory.Error:
					error(message);
					break;
				case ts.DiagnosticCategory.Warning:
					warn(message);
					break;
			}
		});
	}, "transpiling");
}

function stageTranspileJob(dirName, sources)
{
	var targets = [];
	FS.createDirectory(dirName);
	for (var i = 0; i < sources.length; ++i) {
		var fileName = FS.resolve(dirName + '/' + sources[i].name);
		var tool = fileName.endsWith('.mjs') ? ModuleTool : ScriptTool;
		if (fileName.endsWith('.mjs') || fileName.endsWith('.js'))
			fileName = fileName.substring(0, fileName.lastIndexOf('.'));
		fileName += '.js';
		var target = tool.stage(fileName, [ sources[i] ], {
			name: sources[i].name,
		});
		targets[targets.length] = target;
	}
	return targets;
}
