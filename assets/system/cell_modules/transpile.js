/**
 *  Sphere Runtime for Cellscripts
 *  Copyright (c) 2015-2017, Fat Cerberus
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither the name of miniSphere nor the names of its contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
**/

'use strict';
exports = module.exports = transpile;
exports.__esModule = true;
exports.default = exports;

const from  = require('from');
const ts    = require('#/typescript');

const ModuleTool = makeTranspileTool(2.0);
const ScriptTool = makeTranspileTool(1.0);

function transpile(dirName, sources)
{
	return stageTarget(dirName, sources);
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

function stageTarget(dirName, sources)
{
	var targets = [];
	FS.createDirectory(dirName);
	for (var i = 0; i < sources.length; ++i) {
		var fileName = FS.fullPath(dirName + '/' + sources[i].name);
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
