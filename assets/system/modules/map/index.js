/**
 *  miniRT map CommonJS module
 *  (c) 2015-2016 Fat Cerberus
**/

'use strict';
const events  = require('events');
const link    = require('link');
const music   = require('music');
const scenes  = require('scenes');
const threads = require('threads');
const xml     = require('xml');

const Tileset = require('./classes/Tileset');

module.exports =
{
	exit:    exit,
	run:     run,
	Tileset: Tileset,
};

var _isActive = false;
var _tileset = new Tileset('maps/TestvilleTiles.rts');
var _threadID = null;

function exit()
{
	if (_threadID === null)
		throw new Error("map engine is not running");

    _isActive = false;
}

function run(filename)
{
	if (_isActive)
		throw new Error("map engine is already running");

	_isActive = true;
	_threadID = threads.create({
		update:   _update,
		render:   _render,
		getInput: _getInput
	});
	threads.join(_threadID);
	_threadID = null;
}

function _getInput()
{
}

function _loadMap(filename)
{
	
}

function _render()
{
	var tileHeight = _tileset.width;
	var tileWidth = _tileset.height;
	var numAcross = Math.ceil(screen.width / tileWidth) + 1;
	var numDown = Math.ceil(screen.height / tileHeight) + 1;
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
	shape.draw(screen);
}

function _update()
{
	return _isActive;
}
