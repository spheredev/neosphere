'use strict';
module.exports = transpile;

const babel = require('./lib/babel');

const TranspileTool =
new Tool(function(outFileName, inFileNames)
{
	var fileContent = FS.readFile(inFileNames[0]);
	var input = new TextDecoder().decode(fileContent);
	var output = babel.transform(input, {
		presets: [
			[ 'es2015', { modules: false } ],
			'es2016'
		]
	});
	FS.writeFile(outFileName, new TextEncoder().encode(output.code));
}, "transpile");

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
		var target = TranspileTool.build(fileName, [ sources[i] ]);
		targets[targets.length] = target;
	}
	return targets;
}
