/**
 *  Cell Standard Library 'minify' module
 *  (c) 2017 Fat Cerberus
**/

'use strict';
module.exports = minify;

const Babili = require('./lib/babel-minify');

var minifyTool =
new Tool(function(outFileName, inFileNames)
{
	var fileContent = FS.readFile(inFileNames[0]);
	var input = new TextDecoder().decode(fileContent);
	var output = Babili.transform(input);
	FS.writeFile(outFileName, new TextEncoder().encode(output.code));
}, "minifying");

function minify(dirName, sources)
{
	var targets = [];
	FS.createDirectory(dirName);
	for (var i = 0; i < sources.length; ++i) {
		var fileName = FS.resolve(dirName + '/' + sources[i].name);
		var target = minifyTool.stage(fileName, [ sources[i] ], {
			name: sources[i].name,
		});
		targets[targets.length] = target;
	}
	return targets;
}
