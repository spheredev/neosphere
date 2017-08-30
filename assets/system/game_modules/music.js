/**
 *  Sphere Runtime for Sphere games
 *  Copyright (c) 2015-2017, Fat Cerberus
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither the name of miniSphere nor the names of its contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
**/

'use strict';
exports.__esModule = true;
exports.default = exports;

const from    = require('from'),
      Console = require('console'),
      Scene   = require('scene');

var adjuster = null;
var currentSound = null;
var haveOverride = false;
var mixer = new Mixer(44100, 16, 2);
var oldSounds = [];
var topmostSound = null;

Console.defineObject('music', null, {
	override(fileName) { override(fileName); },
	play(fileName) { play(fileName); },
	pop() { pop(); },
	push(fileName) { push(fileName); },
	reset() { reset(); },
	stop() { play(null); },
	volume(value) { adjustVolume(value / 100); },
});

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
function override(fileName, frames)
{
	_crossfade(fileName, frames, true);
	haveOverride = true;
};

exports.play = play;
function play(fileName, fadeTime)
{
	topmostSound = _crossfade(fileName, fadeTime, false);
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
function push(fileName, fadeTime)
{
	var oldSound = topmostSound;
	play(fileName, fadeTime);
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

function _crossfade(fileName, frames, forceChange)
{
	frames = frames !== undefined ? Math.trunc(frames) : 0;

	var allowChange = !haveOverride || forceChange;
	if (currentSound != null && allowChange) {
		currentSound.fader.stop();
		currentSound.fader = new Scene()
			.tween(currentSound.stream, frames, 'linear', { volume: 0.0 })
			.run();
	}
	if (fileName !== null) {
		var fullPath = FS.fullPath(fileName, '@/music');
		fullPath = from([ '', 'ogg', 'mp3', 'it', 'mod', 's3m', 'xm', 'flac' ])
			.select(it => `${fullPath}.${it}`)
			.first(it => FS.fileExists(it))
		if (fullPath === undefined)
			throw new Error(`couldn't find music '${fileName}'`);
		var stream = new Sound(fullPath);
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
