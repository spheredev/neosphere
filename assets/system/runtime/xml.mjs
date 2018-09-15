/**
 *  Sphere Runtime for Sphere games
 *  Copyright (c) 2015-2018, Fat Cerberus
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

const sax = require('#/lib/sax');

export default
class XML
{
	constructor()
	{
		throw new TypeError(`'${new.target.name}' is a static class and cannot be instantiated`);
	}

	static parse(xmlText)
	{
		let dom = { type: 'root', nodes: [] };
		let currentNode = dom;
		let parentNodes = [];

		// parse the XML document using sax-js
		let saxStream = sax.parser(true, { normalize: true });
		saxStream.onopentag = tag => {
			parentNodes.push(currentNode);
			currentNode = { type: 'tag', name: tag.name, nodes: [], attributes: {} };
			for (const key of Object.keys(tag.attributes))
				currentNode.attributes[key] = tag.attributes[key];
		};
		saxStream.onclosetag = tag => {
			let nodeWithTag = currentNode;
			currentNode = parentNodes.pop();
			currentNode.nodes.push(nodeWithTag);
		};
		saxStream.oncomment = text => {
			currentNode.nodes.push({ type: 'comment', text: text });
		};
		saxStream.ontext = text => {
			currentNode.nodes.push({ type: 'text', text: text });
		};
		saxStream.write(xmlText);
		saxStream.close();

		return dom;
	}

	static readFile(fileName)
	{
		let xmlText = FS.readFile(fileName);
		return this.parse(xmlText);
	}
}
