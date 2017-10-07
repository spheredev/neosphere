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

const Babel = require('#/babel-core');

let moduleTool = makeTranspileTool(2),
    scriptTool = makeTranspileTool(1);

export default
function transpile(dirName, sources)
{
	return stageTarget(dirName, sources);
}

function makeTranspileTool(apiVersion)
{
	return new Tool((outFileName, inFileNames) => {
		let sourceType = apiVersion >= 2 ? 'module' : 'script';
		let modules = apiVersion >= 2 ? 'commonjs' : false;
		let input = FS.readFile(inFileNames[0]);
		let output = Babel.transform(input, {
			filename: inFileNames[0],
			presets: [
				[ 'latest', { 'es2015': { modules } } ]
			],
			comments: false,
			retainLines: true,
			sourceMaps: 'inline',
			sourceType,
		});
		FS.writeFile(outFileName, output.code);
	}, "transpiling");
}

function stageTarget(dirName, sources)
{
	let targets = [];
	FS.createDirectory(dirName);
	for (const source of sources) {
		let fileName = FS.fullPath(source.name, dirName);
		let tool = fileName.endsWith('.mjs') ? moduleTool : scriptTool;
		if (fileName.endsWith('.mjs') || fileName.endsWith('.js'))
			fileName = fileName.substring(0, fileName.lastIndexOf('.'));
		fileName += '.js';
		let target = tool.stage(fileName, [ source ], {
			name: source.name,
		});
		targets.push(target);
	}
	return targets;
}
