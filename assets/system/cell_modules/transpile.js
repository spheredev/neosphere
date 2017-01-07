'use strict';
module.exports = main();

const ts = require('./lib/typescript');

const CompilerHost =
{
	fileExists: FS.exists,
	getCurrentDirectory: function() { return "@"; },
	getDefaultLibFileName: function() { return 'lib.d.ts'; },
	getNewLine: function() { return '\n'; },
	readFile,
	useCaseSensitiveFileNames: function() { return true; },
	writeFile,
};

function main()
{
	var TS = new Tool(compileTS, "transpile");
	return {
		TS
	};
}

function compileTS(outName, inNames, options)
{
	if (inNames.length == 0)
		throw new RangeError("one or more sources required");
	var program = ts.createProgram(inNames, {
		module: ts.ModuleKind.CommonJS,
		target: ts.ScriptTarget.ES5,
	}, CompilerHost);
	var result = program.emit();
	
}

function readFile(fileName)
{
	var buffer = FS.readFile(fileName);
	return new TextDecoder().decode(buffer);
}

function writeFile(fileName, text)
{
	var buffer = new TextEncoder().encode(text);
	FS.writeFile(fileName, buffer);
}
