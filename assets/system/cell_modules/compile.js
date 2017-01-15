'use strict';
module.exports =
{
	modules: compileModules,
	scripts: compileScripts,
};

const babel = require('./lib/babel');

const ModuleTool = new Tool(makeTranspiler('commonjs'), "module");
const ScriptTool = new Tool(makeTranspiler(false), "script");

function makeTranspiler(moduleType)
{
	return function(outFileName, inFileNames)
	{
		var fileContent = FS.readFile(inFileNames[0]);
		var input = new TextDecoder().decode(fileContent);
		var output = babel.transform(input, {
			presets: [
				[ 'latest', { es2015: { modules: moduleType } } ],
			],
			comments:    false,
			retainLines: true,
		});
		FS.writeFile(outFileName, new TextEncoder().encode(output.code));
	}
}

function handleCompile(dirName, sources, tool)
{
	var targets = [];
	FS.mkdir(dirName);
	for (var i = 0; i < sources.length; ++i) {
		var fileName = FS.resolve(dirName + '/' + sources[i].name);
		var target = tool.build(fileName, [ sources[i] ]);
		targets[targets.length] = target;
	}
	return targets;
}

function compileModules(dirName, sources)
{
	return handleCompile(dirName, sources, ModuleTool);
}

function compileScripts(dirName, sources)
{
	return handleCompile(dirName, sources, ScriptTool);
}
