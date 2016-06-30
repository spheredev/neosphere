/**
 *  miniRT map CommonJS module
 *  (c) 2015-2016 Fat Cerberus
**/

'use strict';
module.exports =
{
	exit:    exit,
	run:     run,
};

const events  = require('events');
const link    = require('link');
const music   = require('music');
const scenes  = require('scenes');
const threads = require('threads');
const xml     = require('xml');

const Map = require('./Map');

var _isActive = false;
var _map = new Map('maps/Testville.rmp');
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
    _map.draw(screen, 0, 0);
}

function _update()
{
	return _isActive;
}
