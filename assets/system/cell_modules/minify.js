/**
 *  Cell Standard Library 'minify' module
 *  (c) 2017 Fat Cerberus
**/

'use strict';
module.exports = minify;

const Babili = require('./lib/Babili');

var minifyTool =
new Tool(function(outFileName, inFileNames)
{
	var fileContent = FS.readFile(inFileNames[0]);
	var input = new TextDecoder().decode(fileContent);
	var output = Babili.transform(input);
	FS.writeFile(outFileName, new TextEncoder().encode(output.code));
}, "minify");

function minify(dirName, sources)
{
	var targets = [];
	FS.mkdir(dirName);
	for (var i = 0; i < sources.length; ++i) {
		var fileName = FS.resolve(dirName + '/' + sources[i].name);
		var target = minifyTool.stage(fileName, [ sources[i] ]);
		targets[targets.length] = target;
	}
	return targets;
}
