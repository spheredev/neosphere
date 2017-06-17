/**
 *  miniRT joy CommonJS module
 *  (c) 2015-2017 Fat Cerberus
**/

'use strict';
exports.__esModule = true;
exports.default = exports;

const from = require('from');

// historically, Sphere requires a gamepad with at least 2 axes (X/Y) and
// 5 buttons (A, B, X, Y, Start) for full operation.
var devices = from.Array(Joystick.getDevices())
	.where(function(d) { return d.numAxes >= 2; })
	.where(function(d) { return d.numButtons >= 5; })
	.toArray();

Object.defineProperty(exports, 'P1',
{
	enumerable: false, configurable: true,
	get: function() { return devices[0] || Joystick.Null; },
});

Object.defineProperty(exports, 'P2',
{
	enumerable: false, configurable: true,
	get: function() { return devices[1] || Joystick.Null; },
});

Object.defineProperty(exports, 'P3',
{
	enumerable: false, configurable: true,
	get: function() { return devices[2] || Joystick.Null; },
});

Object.defineProperty(exports, 'P4',
{
	enumerable: false, configurable: true,
	get: function() { return devices[3] || Joystick.Null; },
});
