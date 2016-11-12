/**
 *  miniRT joy CommonJS module
 *  (c) 2015-2016 Fat Cerberus
**/

'use strict';
module.exports =
{
	get P1() { return devices[0] || Joystick.Null; },
	get P2() { return devices[1] || Joystick.Null; },
	get P3() { return devices[2] || Joystick.Null; },
	get P4() { return devices[3] || Joystick.Null; },
};

const link = require('link');

// historically, Sphere requires a gamepad with at least 2 axes (X/Y) and
// 5 buttons (A, B, X, Y, Start) for full operation.
var devices = link.Q(Joystick.getDevices())
	.where(function(it) { return it.numAxes >= 2; })
	.where(function(it) { return it.numButtons >= 5; })
	.toArray();
