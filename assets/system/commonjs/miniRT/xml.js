/**
 *  miniRT/xml CommonJS module
 *  a simple XML parser based on the sax-js library.
 *  (c) 2015-2016 Fat Cerberus
**/

if (typeof exports === 'undefined')
{
	throw new TypeError("script must be loaded with require()");
}

const sax = require('sax');

const xml =
module.exports = (function() {
	return {
		load:  load,
		parse: parse,
	};
	
	function load(fileName)
	{
		var file = new FileStream(fileName, 'r');
		var xmlText = file.readString();
		file.close();
		return parse(xmlText);
	}
	
	function parse(xmlText)
	{
		var dom = { type: 'root', nodes: [] };
		var currentNode = dom, parents = [];

		var saxParser = sax.parser(true, { normalize: true });
		saxParser.onopentag = function(tag) {
			parents.push(currentNode);
			currentNode = { type: 'tag', name: tag.name, nodes: [], attributes: {} };
			for (var key in tag.attributes) {
				currentNode.attributes[key] = tag.attributes[key];
			}
		};
		saxParser.onclosetag = function(tag) {
			var nodeWithTag = currentNode;
			currentNode = parents.pop();
			currentNode.nodes.push(nodeWithTag);
			
		};
		saxParser.oncomment = function(text) {
			currentNode.nodes.push({ type: 'comment', text: text });
		}
		saxParser.ontext = function(text) {
			currentNode.nodes.push({ type: 'text', text: text });
		}
		saxParser.write(xmlText);
		saxParser.close();

		return dom;
	}
})();
