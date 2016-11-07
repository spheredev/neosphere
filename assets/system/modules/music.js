/**
 *  miniRT music CommonJS module
 *  (c) 2015-2016 Fat Cerberus
**/

'use strict';
module.exports =
{
	get adjusting() {
		return _isAdjusting();
	},

	adjust:      adjust,
	override:    override,
	play:        play,
	pop:         pop,
	push:        push,
	reset:       reset,
};

const scenes = require('scenes');

var adjuster = null;
var currentSound = null;
var haveOverride = false;
var mixer = new Mixer(44100, 16, 2);
var oldSounds = [];
var topmostSound = null;

// music.adjust()
// Smoothly adjusts the volume of the current BGM.
// Arguments:
//     newVolume: The new volume level, between 0.0 and 1.0 inclusive.
//     frames:    Optional.  The number of frames over which to perform the
//                adjustment. (default: 0).
function adjust(newVolume, frames)
{
	frames = frames !== undefined ? frames >>> 0 : 0;

	newVolume = Math.min(Math.max(newVolume, 0.0), 1.0);
	if (adjuster != null && adjuster.isRunning()) {
		adjuster.stop();
	}
	if (frames > 0) {
		adjuster = new scenes.Scene()
			.tween(mixer, frames, 'linear', { volume: newVolume })
			.run();
	} else {
		mixer.volume = newVolume;
	}
};

// music.override()
// override the BGM with a given track.  push, pop and play operations
// will be deferred until the BGM is reset by calling music.reset().
function override(path, frames)
{
	_crossfade(path, frames, true);
	haveOverride = true;
};

// music.play()
// change the BGM in-place, bypassing the stack.
// Arguments:
//     path:     the SphereFS-compliant path of the sound file to play.  this may be
//               null, in which case the BGM is silenced.
//     fadeTime: optional.  the amount of crossfade to apply, in seconds. (default: 0.0)
function play(path, fadeTime)
{
	topmostSound = _crossfade(path, fadeTime, false);
};

// music.pop()
// pop the previous BGM off the stack and resume playing it.
// arguments:
//     fadeTime: optional.  the amount of crossfade to apply, in seconds. (default: 0.0)
// remarks:
//     if the BGM stack is empty, this is a no-op.
function pop(fadeTime)
{
	fadeTime = fadeTime !== undefined ? fadeTime >>> 0 : 0;
	
	if (oldSounds.length == 0)
		return;
	currentSound.fader.stop();
	currentSound.fader = new scenes.Scene()
		.tween(currentSound.stream, fadeTime, 'linear', { volume: 0.0 })
		.run();
	topmostSound = oldSounds.pop();
	currentSound = topmostSound;
	if (currentSound !== null) {
		currentSound.stream.volume = 0.0;
		currentSound.fader.stop();
		currentSound.fader = new scenes.Scene()
			.tween(currentSound.stream, fadeTime, 'linear', { volume: 1.0 })
			.run();
	}
}

// music.push()
// push the current BGM onto the stack and begin playing another track.  the
// previous BGM can be resumed by calling music.pop().
// arguments:
//     path:     the SphereFS path of the sound file to play
//     fadeTime: optional.  the amount of crossfade to apply, in seconds. (default: 0.0)
function push(path, fadeTime)
{
	var oldSound = topmostSound;
	play(path, fadeTime);
	oldSounds.push(oldSound);
};

// music.reset()
// reset the BGM manager, which removes any outstanding overrides.
function reset(fadeTime)
{
	fadeTime = fadeTime !== undefined ? fadeTime >>> 0 : 0;

	if (!haveOverride)
		return;
	haveOverride = false;

	currentSound.fader.stop();
	currentSound.fader = new scenes.Scene()
		.tween(currentSound.stream, fadeTime, 'linear', { volume: 0.0 })
		.run();
	currentSound = topmostSound;
	if (currentSound !== null) {
		currentSound.stream.volume = 0.0;
		currentSound.fader.stop();
		currentSound.fader = new scenes.Scene()
			.tween(currentSound.stream, fadeTime, 'linear', { volume: 1.0 })
			.run();
	}
};

function _crossfade(path, frames, forceChange)
{
	frames = frames !== undefined ? frames >>> 0 : 0.0;

	var allowChange = !haveOverride || forceChange;
	if (currentSound != null && allowChange) {
		currentSound.fader.stop();
		currentSound.fader = new scenes.Scene()
			.tween(currentSound.stream, frames, 'linear', { volume: 0.0 })
			.run();
	}
	if (path !== null) {
		var stream = new Sound(path);
		stream.repeat = true;
		stream.volume = 0.0;
		stream.play(mixer);
		var fader = new scenes.Scene()
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
