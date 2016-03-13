/**
 *  miniRT/music 2.0 CommonJS module
 *  (c) 2015-2016 Fat Cerberus
 *  a stack-based solution for managing background music.
**/

if (typeof exports === 'undefined')
{
    throw new TypeError("music.js must be loaded using require()");
}

var console = require('./console');
var link    = require('link');
var scenes  = require('./scenes');
var threads = require('./threads');

var music =
module.exports = (function()
{
	var activeSounds = [];
	var adjuster = null;
	var currentSound = null;
	var mixer = new Mixer(44100, 16, 2);
	var oldSounds = [];

	threads.create({ update: update });

	console.register('bgm', null,
	{
		'play': function(trackName, fadeTime) {
			fadeTime = fadeTime !== undefined ? fadeTime : 0.0;
			try { this.play(trackName, fadeTime); }
			catch(e) {
				console.write("Error playing BGM '" + trackName + "'");
			}
		},
		'pop': function(fadeTime) {
			fadeTime = fadeTime !== undefined ? fadeTime : 0.0;
			this.pop(fadeTime);
		},
		'push': function(trackName, fadeTime) {
			fadeTime = fadeTime !== undefined ? fadeTime : 0.0;
			try { this.push(trackName, fadeTime); }
			catch(e) {
				console.write("Error playing BGM '" + trackName + "'");
			}
		},
		'stop': function(fadeTime) {
			fadeTime = fadeTime !== undefined ? fadeTime : 0.0;
			this.play(null, fadeTime);
		},
		'volume': function(volume, fadeTime) {
			fadeTime = fadeTime !== undefined ? fadeTime : 0.0;
			this.adjust(volume, fadeTime);
		},
	});

	return {
		isAdjusting: isAdjusting,
		adjust:      adjust,
		override:    override,
		play:        play,
		pop:         pop,
		push:        push,
	};

	function update()
	{
		for (var i = activeSounds.length - 1; i >= 0; --i) {
			if (activeSounds[i].volume <= 0.0
				&& activeSounds[i] != currentSound
				&& !link(oldSounds).contains(activeSounds[i]))
			{
				activeSounds.splice(i, 1);
			}
		}
		return true;
	};

	// music.isAdjusting()
	// get whether or not the volume level is being adjusted.
	function isAdjusting()
	{
		return adjuster != null && adjuster.isRunning();
	};

	// music.adjust()
	// Smoothly adjusts the volume of the current BGM.
	// Arguments:
	//     newVolume: The new volume level, between 0.0 and 1.0 inclusive.
	//     duration:  Optional. The length of time, in seconds, over which to perform the adjustment.
	//                (default: 0.0).
	function adjust(newVolume, duration)
	{
		duration = duration !== void null ? duration : 0.0;

		newVolume = Math.min(Math.max(newVolume, 0.0), 1.0);
		if (adjuster != null && adjuster.isRunning()) {
			adjuster.stop();
		}
		if (duration > 0.0) {
			adjuster = new scenes.Scene()
				.tween(mixer, duration, 'linear', { volume: newVolume })
				.run();
		} else {
			mixer.volume = newVolume;
		}
	};

	// music.override()
	// override the BGM with a given track.  push, pop and play operations
	// will be deferred until the BGM is reset by calling music.reset().
	function override(path, fadeTime)
	{
		Abort("music.override(): not implemented", -1);
	};

	// music.play()
	// change the BGM in-place, bypassing the stack.
	// Arguments:
	//     path:     the SphereFS-compliant path of the sound file to play.  this may be
	//               null, in which case the BGM is silenced.
	//     fadeTime: optional.  the amount of crossfade to apply, in seconds. (default: 0.0)
	function play(path, fadeTime)
	{
		if (fadeTime === undefined) fadeTime = 0.0;

		if (currentSound != null) {
			currentSound.fader.stop();
			currentSound.fader = new scenes.Scene()
				.tween(currentSound.stream, fadeTime, 'linear', { volume: 0.0 })
				.run();
		}
		if (path !== null) {
			var stream = new Sound(path, mixer);
			stream.volume = 0.0;
			stream.play(true);
			var fader = new scenes.Scene()
				.tween(stream, fadeTime, 'linear', { volume: 1.0 })
				.run();
			currentSound = { stream: stream, fader: fader };
			activeSounds.push(currentSound);
		}
	};

	// music.pop()
	// pop the previous BGM off the stack and resume playing it.
	// arguments:
	//     fadeTime: optional.  the amount of crossfade to apply, in seconds. (default: 0.0)
	// remarks:
	//     if the BGM stack is empty, this is a no-op.
	function pop(fadeTime)
	{
		if (fadeTime === undefined) fadeTime = 0.0;

		if (oldSounds.length == 0) return;
		currentSound.fader.stop();
		currentSound.fader = new scenes.Scene()
			.tween(currentSound.stream, fadeTime, 'linear', { volume: 0.0 })
			.run();
		var oldSound = oldSounds.pop();
		currentSound = oldSound;
		if (currentSound !== null) {
			currentSound.stream.volume = 0.0;
			currentSound.stream.play();
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
	//     path:     the path to the sound file to play, relative to ~sgm/sounds.
	//     fadeTime: optional.  the amount of crossfade to apply, in seconds. (default: 0.0)
	function push(path, fadeTime)
	{
		var oldSound = currentSound;
		this.play(path, fadeTime);
		oldSounds.push(oldSound);
	};
})();
