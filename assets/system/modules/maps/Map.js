'use strict';
module.exports = Map;

const Tileset = require('./Tileset');

function Map(filename)
{
	var _persons = [];
	var _tileset = new Tileset('maps/TestvilleTiles.rts');

	this.addPerson = _addPerson;
	this.draw = _draw;

	function _addPerson(person)
	{
		persons.push(person);
	}

	function _draw(surface, cameraX, cameraY)
	{
		var tileHeight = _tileset.width;
		var tileWidth = _tileset.height;
		var numAcross = Math.ceil(surface.width / tileWidth) + 1;
		var numDown = Math.ceil(surface.height / tileHeight) + 1;
		var vlist = [], i = 0;
		for (var iY = 0; iY < numDown; ++iY) {
			for (var iX = 0; iX < numAcross; ++iX) {
				var uv = _tileset.uv(5);
				var x1 = iX * tileWidth;
				var y1 = iY * tileHeight;
				var x2 = x1 + tileWidth;
				var y2 = y1 + tileHeight;
				vlist[i++] = { x: x1, y: y1, u: uv.u1, v: uv.v1 };
				vlist[i++] = { x: x2, y: y1, u: uv.u2, v: uv.v1 };
				vlist[i++] = { x: x1, y: y2, u: uv.u1, v: uv.v2 };
				vlist[i++] = { x: x2, y: y2, u: uv.u2, v: uv.v2 };
				vlist[i++] = { x: x2, y: y2, u: uv.u1, v: uv.v2 };
			}
		}
		var shape = new Shape(vlist, _tileset.texture, ShapeType.TriStrip);
		shape.draw(surface);
	}
}
