/**
 * minisphere Runtime 1.1b4 - (c) 2015 Fat Cerberus
 * A set of system scripts providing advanced, high-level functionality not
 * available in the engine itself.
 *
 * [mini/BGM.js]
 * A stack-based solution for managing background music.
**/

RequireSystemScript('mini/Core.js');
RequireSystemScript('mini/Threads.js');

// mini.BGM
// Encapsulates the BGM manager.
mini.BGM = new (function()
{
	this.adjuster = null;
	this.stream = null;
	this.volume = 1.0;
	this.stack = [];
})();
	
// miniBGM initializer
// Prepares miniBGM for use when the user calls mini.initialize().
mini.onStartUp.add(mini.BGM, function()
{
	Print("mini: Initializing miniBGM");
	mini.Threads.create(this);
});
	
// mini.BGM.update()
// Updates the BGM thread per frame.
mini.BGM.update = function()
{
	if (this.stream != null) {
		this.stream.setVolume(this.volume * 255);
	}
	if (this.oldStream != null) {
		this.oldStream.setVolume(this.volume * 255);
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
			.tween(this, duration, 'linear', { volume: newVolume })
			.run();
	} else {
		this.volume = newVolume;
		if (this.stream != null) {
			this.stream.setVolume(this.volume * 255);
		}
	}
};

// .play() method
// Changes the BGM in place, bypassing the stack.
// Arguments:
//     path: The path to the sound file to play, relative to <game_dir>/sounds. This
//           can be null, in which case the BGM is silenced.
mini.BGM.play = function(path)
{
	if (this.stream !== null) {
		this.stream.stop();
	}
	if (path !== null) {
		this.stream = new Sound(path, true);
		this.stream.setVolume(this.volume * 255);
		this.stream.play(true);
	} else {
		this.stream = null;
	}
};

// .push() method
// Pushes the current BGM onto the stack and begins playing another track. The
// previous BGM can be resumed via .pop().
// Arguments:
//     path: The path to the sound file to play, relative to <game_dir>/sounds.
mini.BGM.push = function(path)
{
	var oldStream = this.stream;
	if (oldStream !== null) {
		oldStream.pause();
	}
	this.stream = null;
	this.play(path);
	this.stack.push({
		stream: oldStream
	});
};

// .pop() method
// Pops the previous BGM off the stack and resumes it.
// Remarks:
//     This does nothing if the BGM stack is empty.
mini.BGM.pop = function()
{
	if (this.stack.length == 0) return;
	this.stream.stop();
	var oldBGM = this.stack.pop();
	this.stream = oldBGM.stream;
	if (this.stream !== null) {
		this.stream.setVolume(this.volume * 255);
		this.stream.play();
	}
}
