'use strict';
module.exports = Tileset;

const bin   = require('bin');
const Atlas = require('./Atlas');

function Tileset(filename)
{
	const schema =
	[
		{ id: 'signature', type: 'fstring', size: 4, regex: '^\.rts$' },
		{ id: 'version', type: 'uintLE', size: 2, range: [ 1, 1 ] },
		{ id: 'numTiles', type: 'uintLE', size: 2 },
		{ id: 'tileWidth', type: 'uintLE', size: 2 },
		{ id: 'tileHeight', type: 'uintLE', size: 2 },
		{ id: 'bpp', type: 'uintLE', size: 2, values: [ 32 ] },
		{ id: 'compressed', type: 'bool', values: [ false ] },
		{ id: 'obstructed', type: 'bool' },
		{ type: 'reserved', size: 240 },
		{ id: 'tiles', type: 'array', count: '@numTiles', subtype: {
			type: 'raw', size: function() { return 4 * this.tileWidth * this.tileHeight; }
		}},
		{ id: 'tileData', type: 'array', count: '@numTiles', subtype: { type: 'object', schema: [
			{ type: 'reserved', size: 1 },
			{ id: 'animated', type: 'bool' },
			{ id: 'nextTile', type: 'uintLE', size: 2 },
			{ id: 'delay', type: 'uintLE', size: 2 },
			{ type: 'reserved', size: 1 },
			{ id: 'obsType', type: 'uintLE', size: 1 },
			{ id: 'numSegments', type: 'uintLE', size: 2 },
			{ type: 'reserved', size: 22 },
			{ id: 'obsMap', type: 'switch', field: 'obsType', cases: [
				{ value: 0, type: 'reserved', size: 0 },
				{ value: 1, type: 'raw', size: function() { return this.tileWidth * this.tileHeight } },
				{ value: 2, type: 'object', schema: [
					{ id: 'x1', type: 'uintLE', size: 2 },
					{ id: 'y1', type: 'uintLE', size: 2 },
					{ id: 'x2', type: 'uintLE', size: 2 },
					{ id: 'y2', type: 'uintLE', size: 2 },
				]},
			]},
		]}},
	];

	var rts = bin.load(filename, schema);
	var atlas = new Atlas(rts.tileWidth, rts.tileHeight, rts.numTiles);
	for (var i = 0; i < rts.numTiles; ++i)
		atlas.blit(i, rts.tiles[i]);

	Object.defineProperty(this, 'height', {
		get: function() { return atlas.cellHeight; }
	});

	Object.defineProperty(this, 'texture', {
		get: function() { return atlas.toImage(); }
	});

	Object.defineProperty(this, 'width', {
		get: function() { return atlas.cellWidth; }
	});

	this.uv = function uv(index)
	{
		return atlas.uv(index);
	};
}
