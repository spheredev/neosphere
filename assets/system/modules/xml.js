/**
 *  miniRT xml CommonJS module
 *  (c) 2015-2016 Fat Cerberus
**/

'use strict';
module.exports =
{
	load:  load,
	parse: parse,
};

const sax = require('./lib/sax');

function load(fileName)
{
	var buffer = FS.readFile(fileName);
	var xmlText = new TextDecoder().decode(buffer);
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
