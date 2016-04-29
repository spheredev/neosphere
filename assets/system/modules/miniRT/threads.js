/**
 *  miniRT/threads CommonJS module
 *  a threading engine for Sphere with an API similar to Pthreads
 *  (c) 2015-2016 Fat Cerberus
**/

if (typeof exports === 'undefined')
{
    throw new TypeError("script must be loaded with require()");
}

const link = require('link');

// threads object
// represents the threader.
var threads =
module.exports = (function()
{
	var _SetFrameRate = SetFrameRate;
	var _SetMapEngineFrameRate = SetMapEngineFrameRate;
	var _SetRenderScript = SetRenderScript;
	var _SetUpdateScript = SetUpdateScript;
	var _MapEngine = MapEngine;
	
	var currentSelf = 0;
	var hasUpdated = false;
	var frameRate = 60;
	var manifest = GetGameManifest();
	var nextThreadID = 1;
	var threads = [];
	
	if ('frameRate' in manifest && typeof manifest.frameRate === 'number')
		frameRate = manifest.frameRate;
	var threadSorter = function(a, b) {
		return a.priority != b.priority ?
			a.priority - b.priority :
			a.id - b.id;
	};
	
	_SetUpdateScript(updateAll);
	_SetRenderScript(renderAll);
	_SetFrameRate(frameRate);
	_SetMapEngineFrameRate(frameRate);

	// the threading system commandeers several legacy APIs, making them unsafe
	// to use without messing with the operation of miniRT, so we disable them.
	// for MapEngine(), just override the framerate.
	MapEngine = function(mapName) { _MapEngine(mapName, frameRate); }
	SetUpdateScript = SetRenderScript = SetFrameRate = SetMapEngineFrameRate
		= function() { Abort("API incompatible with miniRT/threads", -1); }

	return {
		get frameRate() { return frameRate; },
		set frameRate(value) { set_frameRate(value); },
		create:    create,
		createEx:  createEx,
		isRunning: isRunning,
		join:      join,
		kill:      kill,
		renderAll: renderAll,
		self:      self,
		updateAll: updateAll,
	};

	function doFrame()
	{
		if (IsMapEngineRunning())
			RenderMap();
		else
			renderAll();
		FlipScreen();
		if (IsMapEngineRunning()) {
			hasUpdated = false;
			UpdateMapEngine();
			if (!hasUpdated)
				updateAll();
		} else {
			updateAll();
		}
	};

	// threads.frameRate (read/write)
	// gets or sets the miniRT frame rate.  this should be used instead of
	// SetFrameRate(), which will throw an error if called.
	function set_frameRate(value)
	{
		frameRate = Math.floor(Math.max(value, 1));
		_SetFrameRate(frameRate);
		_SetMapEngineFrameRate(frameRate);
	}
	
	// threads.create()
	// create an object thread.  this is the recommended thread creation method.
	// arguments:
	//     entity:   the object for which to create the thread.  this object's .update() method
	//               will be called once per frame, along with .render() and .getInput() if they
	//               exist, until .update() returns false.
	//     priority: optional.  the render priority for the new thread.  higher-priority threads are rendered
	//               later in a frame than lower-priority ones.  ignored if no renderer is provided. (default: 0)
	function create(entity, priority)
	{
		Assert(entity instanceof Object || entity === null, "create() argument must be a valid object", -1);

		priority = priority !== undefined ? priority : 0;

		var update = entity.update;
		var render = (typeof entity.render === 'function') ? entity.render : undefined;
		var getInput = (typeof entity.getInput === 'function') ? entity.getInput : null;
		return createEx(entity, {
			priority: priority,
			update: entity.update,
			render: entity.render,
			getInput: entity.getInput,
		});
	};

	// threads.createEx()
	// create a thread with advanced options.
	// arguments:
	//     that:       the object to bind as 'this' to thread callbacks.  may be null.
	//     threadDesc: an object describing the thread.  this should contain the following members:
	//                     update:   the update function for the new thread.
	//                     render:   optional.  the render function for the new thread.
	//                     getInput: optional.  the input handler for the new thread.
	//                     priority: optional.  the render priority for the new thread.  higher-priority threads
	//                               are rendered later in a frame than lower-priority ones.  ignored if no
	//                               renderer is provided. (default: 0)
	// remarks:
	//     this is for advanced thread creation.  for typical use, it is recommended to use
	//     threads.create() instead.
	function createEx(that, threadDesc)
	{
		Assert(arguments.length >= 2, "threads.createEx() expects 2 arguments", -1);

		var update = threadDesc.update.bind(that);
		var render = typeof threadDesc.render === 'function'
			? threadDesc.render.bind(that) : undefined;
		var getInput = typeof threadDesc.getInput === 'function'
			? threadDesc.getInput.bind(that) : undefined;
		var priority = 'priority' in threadDesc ? threadDesc.priority : 0;
		var newThread = {
			isValid: true,
			id: nextThreadID++,
			that: that,
			inputHandler: getInput,
			isBusy: false,
			priority: priority,
			renderer: render,
			updater: update,
		};
		threads.push(newThread);
		return newThread.id;
	};

	// threads.isRunning()
	// determine whether a thread is still running.
	// arguments:
	//     threadID: the ID of the thread to check.
	function isRunning(threadID)
	{
		if (threadID == 0) return false;
		for (var i = 0; i < threads.length; ++i) {
			if (threads[i].id == threadID) {
				return true;
			}
		}
		return false;
	};

	// threads.join()
	// Blocks the calling thread until one or more other threads have terminated.
	// arguments:
	//     threadID: either a single thread ID or an array of them.  any invalid thread
	//               ID will cause an error to be thrown.
	function join(threadIDs)
	{
		threadIDs = threadIDs instanceof Array ? threadIDs : [ threadIDs ];
		while (link(threads)
			.filterBy('id', threadIDs)
			.length() > 0)
		{
			doFrame();
		}
	};

	// threads.kill()
	// forcibly terminate a thread.
	// arguments:
	//     threadID: the ID of the thread to terminate.
	function kill(threadID)
	{
		link(threads)
			.where(function(thread) { return thread.id == threadID })
			.execute(function(thread) { thread.isValid = false; })
			.remove();
	};
	// threads.renderAll()
	// Renders the current frame by calling all active threads' renderers.
	function renderAll()
	{
		if (IsSkippedFrame()) return;
		link(link(threads).sort(threadSorter))
			.where(function(thread) { return thread.isValid; })
			.where(function(thread) { return thread.renderer !== undefined; })
			.each(function(thread)
		{
			thread.renderer();
		}.bind(this));
	};

	// threads.self()
	// get the currently executing thread's thread ID.
	// Remarks:
	//     if this function is used outside of a thread update, render or input
	//     handling call, it will return 0 (the ID of the main thread).
	function self()
	{
		return currentSelf;
	};

	// threads.updateAll()
	// update all active threads for the next frame.
	function updateAll(threadID)
	{
		var threadsEnding = [];
		link(link(threads).toArray())
			.where(function(thread) { return thread.isValid; })
			.where(function(thread) { return !thread.isBusy; })
			.each(function(thread)
		{
			var lastSelf = currentSelf;
			thread.isBusy = true;
			currentSelf = thread.id;
			var isRunning = thread.updater(thread.id);
			if (thread.inputHandler !== undefined && isRunning)
				thread.inputHandler();
			currentSelf = lastSelf;
			thread.isBusy = false;
			if (!isRunning) threadsEnding.push(thread.id);
		}.bind(this));
		link(threadsEnding)
			.each(function(threadID)
		{
			kill(threadID);
		});
		hasUpdated = true;
	}
})();
