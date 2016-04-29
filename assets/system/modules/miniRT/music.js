/**
 *  miniRT/music CommonJS module
 *  a stack-based solution for managing background music
 *  (c) 2015-2016 Fat Cerberus
**/

if (typeof exports === 'undefined')
{
    throw new TypeError("script must be loaded with require()");
}

const console = require('./console');
const scenes  = require('./scenes');

var music =
module.exports = (function()
{
	var adjuster = null;
	var currentSound = null;
	var haveOverride = false;
	var mixer = new Mixer(44100, 16, 2);
	var oldSounds = [];
	var topmostSound = null;

	console.register('bgm', null,
	{
		'adjust': function(volume, fadeTime) {
			adjust(volume, fadeTime);
		},
		'override': function(trackName, fadeTime) {
			try { override(trackName, fadeTime); }
			catch(e) {
				console.write("error playing `" + trackName + "`");
			}
		},
		'play': function(trackName, fadeTime) {
			try { play(trackName, fadeTime); }
			catch(e) {
				console.write("error playing `" + trackName + "`");
			}
		},
		'pop': function(fadeTime) {
			pop(fadeTime);
		},
		'push': function(trackName, fadeTime) {
			try { push(trackName, fadeTime); }
			catch(e) {
				console.write("error playing `" + trackName + "`");
			}
		},
		'reset': function(fadeTime) {
			reset(fadeTime);
		},
		'stop': function(fadeTime) {
			play(null, fadeTime);
		},
	});

	return {
		isAdjusting: isAdjusting,
		adjust:      adjust,
		override:    override,
		play:        play,
		pop:         pop,
		push:        push,
		reset:       reset,
	};

	function crossfade(path, fadeTime, forceChange)
	{
		fadeTime = fadeTime !== undefined ? fadeTime : 0.0;
		
		var allowChange = !haveOverride || forceChange;
		if (currentSound != null && allowChange) {
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
		duration = duration !== undefined ? duration : 0.0;

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
		crossfade(path, fadeTime, true);
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
		fadeTime = fadeTime !== undefined ? fadeTime : 0.0;

		topmostSound = crossfade(path, fadeTime, false);
	};

	// music.pop()
	// pop the previous BGM off the stack and resume playing it.
	// arguments:
	//     fadeTime: optional.  the amount of crossfade to apply, in seconds. (default: 0.0)
	// remarks:
	//     if the BGM stack is empty, this is a no-op.
	function pop(fadeTime)
	{
		fadeTime = fadeTime !== undefined ? fadeTime : 0.0;

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
		var oldSound = topmostSound;
		play(path, fadeTime);
		oldSounds.push(oldSound);
	};

	// music.reset()
	// reset the BGM manager, which removes any outstanding overrides.
	function reset(fadeTime)
	{
		fadeTime = fadeTime !== undefined ? fadeTime : 0.0;
		
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
			currentSound.stream.play();
			currentSound.fader.stop();
			currentSound.fader = new scenes.Scene()
				.tween(currentSound.stream, fadeTime, 'linear', { volume: 1.0 })
				.run();
		}
	};
})();
