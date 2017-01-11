'use strict';
module.exports = transpile;

const ts = require('./lib/typescriptServices');

const CompilerHost =
{
	fileExists: FS.exists,
	getCanonicalFileName(fileName) { return FS.resolve(fileName); },
	getCurrentDirectory() { return '.'; },
	getDefaultLibFileName() { return 'lib.d.ts' },
	getNewLine() { return '\n'; },
	getSourceFile(fileName, langVersion) {
		try {
			var text = this.readFile(fileName);
			return ts.createSourceFile(fileName, text, langVersion);
		}
		catch(e) {
			return undefined;
		}
	},
	readFile(fileName) {
		var data = FS.readFile(fileName);
		return new TextDecoder().decode(data);
	},
	useCaseSensitiveFileNames() { return true; },
	writeFile(fileName, text) {
		var data = new TextEncoder().encode(text);
		FS.writeFile(fileName, data);
	},
};

const TSCompiler =
new Tool(function(outName, inNames) {
	var program = ts.createProgram(inNames, {
		module:  ts.ModuleKind.CommonJS,
		target:  ts.ScriptTarget.ES5,
		outFile: outName,
	}, CompilerHost);
	program.emit();
}, "compile");

function transpile(dirName, sources)
{
	var targets = [];
	FS.mkdir(dirName);
	for (var i = 0; i < sources.length; ++i) {
		var fileName = FS.resolve(dirName + '/' + sources[i].name);
		var extStart = fileName.lastIndexOf('.');
		fileName = extStart !== -1
			? fileName.substring(0, extStart) + '.js'
			: fileName + '.js';
		var target = TSCompiler.build(fileName, [ sources[i] ]);
		targets[targets.length] = target;
	}
	return targets;
}
