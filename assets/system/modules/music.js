/**
 *  miniRT music CommonJS module
 *  (c) 2015-2016 Fat Cerberus
**/

'use strict';
exports.__esModule = true;
exports.default = exports;

const Scene = require('scenes').Scene;

var adjuster = null;
var currentSound = null;
var haveOverride = false;
var mixer = new Mixer(44100, 16, 2);
var oldSounds = [];
var topmostSound = null;

Object.defineProperty(exports, 'adjusting',
{
	enumerable: true, configurable: true,
	get: function() { return _isAdjusting(); }
});

exports.adjustVolume = adjustVolume;
function adjustVolume(newVolume, frames)
{
	frames = frames !== undefined ? Math.trunc(frames) : 0;

	newVolume = Math.min(Math.max(newVolume, 0.0), 1.0);
	if (_isAdjusting())
		adjuster.stop();
	if (frames > 0) {
		adjuster = new Scene()
			.tween(mixer, frames, 'linear', { volume: newVolume })
			.run();
	} else {
		mixer.volume = newVolume;
	}
};

exports.override = override;
function override(path, frames)
{
	_crossfade(path, frames, true);
	haveOverride = true;
};

exports.play = play;
function play(path, fadeTime)
{
	topmostSound = _crossfade(path, fadeTime, false);
};

exports.pop = pop;
function pop(fadeTime)
{
	fadeTime = fadeTime !== undefined ? Math.trunc(fadeTime) : 0;
	
	if (oldSounds.length == 0)
		return;
	currentSound.fader.stop();
	currentSound.fader = new Scene()
		.tween(currentSound.stream, fadeTime, 'linear', { volume: 0.0 })
		.run();
	topmostSound = oldSounds.pop();
	currentSound = topmostSound;
	if (currentSound !== null) {
		currentSound.stream.volume = 0.0;
		currentSound.fader.stop();
		currentSound.fader = new Scene()
			.tween(currentSound.stream, fadeTime, 'linear', { volume: 1.0 })
			.run();
	}
}

exports.push = push;
function push(path, fadeTime)
{
	var oldSound = topmostSound;
	play(path, fadeTime);
	oldSounds.push(oldSound);
};

exports.reset = reset;
function reset(fadeTime)
{
	fadeTime = fadeTime !== undefined ? Math.trunc(fadeTime) : 0;

	if (!haveOverride)
		return;
	haveOverride = false;

	currentSound.fader.stop();
	currentSound.fader = new Scene()
		.tween(currentSound.stream, fadeTime, 'linear', { volume: 0.0 })
		.run();
	currentSound = topmostSound;
	if (currentSound !== null) {
		currentSound.stream.volume = 0.0;
		currentSound.fader.stop();
		currentSound.fader = new Scene()
			.tween(currentSound.stream, fadeTime, 'linear', { volume: 1.0 })
			.run();
	}
};

function _crossfade(path, frames, forceChange)
{
	frames = frames !== undefined ? Math.trunc(frames) : 0;

	var allowChange = !haveOverride || forceChange;
	if (currentSound != null && allowChange) {
		currentSound.fader.stop();
		currentSound.fader = new Scene()
			.tween(currentSound.stream, frames, 'linear', { volume: 0.0 })
			.run();
	}
	if (path !== null) {
		var stream = new Sound(path);
		stream.repeat = true;
		stream.volume = 0.0;
		stream.play(mixer);
		var fader = new Scene()
			.tween(stream, frames, 'linear', { volume: 1.0 })
		var newSound = { stream: stream, fader: fader };
		if (allowChange) {
			currentSound = newSound;
			newSound.fader.run();
		}
		return newSound;
	}
	else {
		return null;
	}
}

function _isAdjusting()
{
	return adjuster != null && adjuster.isRunning();
};
