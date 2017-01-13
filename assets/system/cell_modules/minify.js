'use strict';
module.exports = minify;

require('polyfill');
const babili = require('./lib/babili');

const MinifyTool =
new Tool(function(outFileName, inFileNames)
{
	var fileContent = FS.readFile(inFileNames[0]);
	var input = new TextDecoder().decode(fileContent);
	var output = babili.transform(input);
	FS.writeFile(outFileName, new TextEncoder().encode(output.code));
}, "minify");

function minify(dirName, sources)
{
	var targets = [];
	FS.mkdir(dirName);
	for (var i = 0; i < sources.length; ++i) {
		var fileName = FS.resolve(dirName + '/' + sources[i].name);
		var target = MinifyTool.build(fileName, [ sources[i] ]);
		targets[targets.length] = target;
	}
	return targets;
}
