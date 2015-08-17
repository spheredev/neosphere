/**
 * miniRT  (c) 2015 Fat Cerberus
 * A set of system scripts for minisphere providing advanced, high-level
 * functionality not available in the engine itself.
 *
 * [mini/miniBGM.js]
 * A stack-based solution for managing background music.
**/

if (typeof mini === 'undefined') {
    Abort("miniRT component script; use miniRT.js instead", -2);
}

RequireSystemScript('mini/miniConsole.js');
RequireSystemScript('mini/miniThreads.js');

// mini.BGM
// Encapsulates the BGM manager.
mini.BGM = new (function()
{
	this.adjuster = null;
	this.currentSound = null;
	this.oldSounds = [];
	this.activeSounds = [];
})();
	
// miniBGM initializer
// Prepares miniBGM for use when the user calls mini.initialize().
mini.onStartUp.add(mini.BGM, function()
{
	Print("Initializing miniRT BGM manager");
	this.mixer = new Mixer(44100, 16, 2);
	mini.Threads.create(this);
	mini.Console.register('bgm', mini.BGM, {
		'play': function(trackName) {
			try { this.play(trackName); }
			catch(e) {
				mini.Console.write("Error playing BGM '" + trackName + "'");
			}
		},
		'pop': function() { this.pop(); },
		'push': function(trackName) {
			try { this.push(trackName); }
			catch(e) {
				mini.Console.write("Error playing BGM '" + trackName + "'");
			}
		},
		'stop': function() { this.play(null); },
		'volume': function(volume) { this.adjust(volume); },
	});
});
	
mini.BGM.update = function()
{
	for (var i = this.activeSounds.length - 1; i >= 0; --i) {
		if (this.activeSounds[i].volume <= 0.0
		    && this.activeSounds[i] != this.currentSound
		    && !mini.Link(this.oldSounds).contains(this.activeSounds[i]))
		{
			this.activeSounds.splice(i, 1);
		}
	}
	return true;
};

// mini.BGM.isAdjusting()
// Returns whether or not the volume level is being adjusted.
mini.BGM.isAdjusting = function()
{
	return this.adjuster != null && this.adjuster.isRunning();
};

// mini.BGM.adjust()
// Smoothly adjusts the volume of the current BGM.
// Arguments:
//     newVolume: The new volume level, between 0.0 and 1.0 inclusive.
//     duration:  Optional. The length of time, in seconds, over which to perform the adjustment.
//                (default: 0.0).
mini.BGM.adjust = function(newVolume, duration)
{
	duration = duration !== void null ? duration : 0.0;
	
	newVolume = Math.min(Math.max(newVolume, 0.0), 1.0);
	if (this.adjuster != null && this.adjuster.isRunning()) {
		this.adjuster.stop();
	}
	if (duration > 0.0) {
		this.adjuster = new mini.Scene()
			.tween(this.mixer, duration, 'linear', { volume: newVolume })
			.run();
	} else {
		this.mixer.volume = newVolume;
	}
};

// .override() method
// Overrides the BGM with a given song. Push, pop and play operations
// will be deferred until the BGM is reset by calling BGM.reset().
mini.BGM.override = function(path, fadeTime)
{
};

// .play() method
// Changes the BGM in place, bypassing the stack.
// Arguments:
//     path:     The path to the sound file to play, relative to <game_dir>/sounds. This
//               can be null, in which case the BGM is silenced.
//     fadeTime: Optional. The amount of crossfade to apply, in seconds. (default: 0.0)
mini.BGM.play = function(path, fadeTime)
{
	if (fadeTime === undefined) fadeTime = 0.0;
	
	if (this.currentSound != null) {
		this.currentSound.fader.stop();
		this.currentSound.fader = new mini.Scene()
			.tween(this.currentSound.stream, fadeTime, 'linear', { volume: 0.0 })
			.run();
	}
	if (path !== null) {
		var stream = new Sound(path, this.mixer);
		stream.volume = 0.0;
		stream.play(true);
		var fader = new mini.Scene()
			.tween(stream, fadeTime, 'linear', { volume: 1.0 })
			.run();
		this.currentSound = { stream: stream, fader: fader };
		this.activeSounds.push(this.currentSound);
	}
};

// .push() method
// Pushes the current BGM onto the stack and begins playing another track. The
// previous BGM can be resumed via .pop().
// Arguments:
//     path:     The path to the sound file to play, relative to <game_dir>/sounds.
//     fadeTime: Optional. The amount of crossfade to apply, in seconds. (default: 0.0)
mini.BGM.push = function(path, fadeTime)
{
	var oldSound = this.currentSound;
	this.play(path, fadeTime);
	this.oldSounds.push(oldSound);
};

// .pop() method
// Pops the previous BGM off the stack and resumes it.
// Arguments:
//     fadeTime: Optional. The amount of crossfade to apply, in seconds. (default: 0.0)
// Remarks:
//     If the BGM stack is empty, this is a no-op.
mini.BGM.pop = function(fadeTime)
{
	if (fadeTime === undefined) fadeTime = 0.0;
	
	if (this.oldSounds.length == 0) return;
	this.currentSound.fader.stop();
	this.currentSound.fader = new mini.Scene()
		.tween(this.currentSound.stream, fadeTime, 'linear', { volume: 0.0 })
		.run();
	var oldSound = this.oldSounds.pop();
	this.currentSound = oldSound;
	if (this.currentSound !== null) {
		this.currentSound.stream.volume = 0.0;
		this.currentSound.stream.play();
		this.currentSound.fader.stop();
		this.currentSound.fader = new mini.Scene()
			.tween(this.currentSound.stream, fadeTime, 'linear', { volume: 1.0 })
			.run();
	}
}
