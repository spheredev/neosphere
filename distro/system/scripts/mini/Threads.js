/**
 * minisphere Runtime 1.1b3 - (c) 2015 Fat Cerberus
 * A set of system scripts providing advanced, high-level functionality not
 * available in the engine itself.
 *
 * [mini/Threads.js]
 * A cooperative threader with a similar API to pthreads, which replaces
 * Sphere's update and render scripts with a much more robust solution.
**/

RequireSystemScript('mini/Core.js');
RequireSystemScript('mini/Link.js');

// Threads object
// Encapsulates the thread manager.
mini.Threads = new (function()
{
	this.isInitialized = false;
	this.currentSelf = 0;
	this.hasUpdated = false;
	this.nextThreadID = 1;
	this.threads = [];
})();

// initializer registration
// Initializes Threads when the user calls mini.initialize().
mini.startup.add(mini.Threads, function(params)
{
	Print("mini: Initializing mini.Threads");
	
	this.threadSorter = function(a, b) {
		return a.priority != b.priority ?
			a.priority - b.priority :
			a.id - b.id;
	};
	SetUpdateScript('mini.Threads.updateAll();');
	SetRenderScript('mini.Threads.renderAll();');
	this.isInitialized = true;
});

// mini.Threads.create()
// Creates an entity thread. This is the recommended method for
// creating persistent threads.
// Arguments:
//     entity:   The object for which to create the thread. This object's .update() method
//               will be called once per frame, along with .render() and .getInput() if they
//               exist, until .update() returns false.
//     priority: Optional. The render priority for the new thread. Higher-priority threads are rendered
//               later in a frame than lower-priority ones. Ignored if no renderer is provided. (default: 0)
mini.Threads.create = function(entity, priority)
{
	if (!this.isInitialized)
		Abort("mini.Threads.create(): must call mini.initialize() first", -1);
	
	priority = priority !== undefined ? priority : 0;
	
	var updater = entity.update;
	var renderer = (typeof entity.render === 'function') ? entity.render : null;
	var inputHandler = (typeof entity.getInput === 'function') ? entity.getInput : null;
	var threadDesc = {
		priority: priority,
		update: entity.update
	};
	if (typeof entity.render === 'function')
		threadDesc.render = entity.render;
	if (typeof entity.getInput === 'function')
		threadDesc.getInput = entity.getInput;
	return this.createEx(entity, threadDesc);
};

// mini.Threads.createEx()
// Creates a thread and begins running it.
// Arguments:
//     o:          The object to pass as 'this' to the updater/renderer/input handler. May be null.
//     threadDesc: An object describing the thread. This should contain the following members:
//                     update:   The update function for the new thread.
//                     render:   Optional. The render function for the new thread.
//                     getInput: Optional. The input handler for the new thread.
//                     priority: Optional. The render priority for the new thread. Higher-priority threads
//                               are rendered later in a frame than lower-priority ones. Ignored if no
//                               renderer is provided. (default: 0)
// Remarks:
//     This is for advanced thread creation. For typical use, it is recommended to use
//     Threads.create() instead.
mini.Threads.createEx = function(o, threadDesc)
{
	if (!this.isInitialized)
		Abort("mini.Threads.createEx(): must call mini.initialize() first", -1);
	updater = threadDesc.update.bind(o);
	renderer = 'render' in threadDesc ? threadDesc.render.bind(o) : null;
	inputHandler = 'getInput' in threadDesc ? threadDesc.getInput.bind(o) : null;
	priority = 'priority' in threadDesc ? threadDesc.priority : 0;
	var newThread = {
		id: this.nextThreadID++,
		isValid: true,
		inputHandler: inputHandler,
		isUpdating: false,
		priority: priority,
		renderer: renderer,
		updater: updater,
		isPaused: false,
	};
	this.threads.push(newThread);
	return newThread.id;
};

// .isRunning() method
// Determines whether a thread is still running.
// Arguments:
//     threadID: The ID of the thread to check.
mini.Threads.isRunning = function(threadID)
{
	if (!this.isInitialized)
		Abort("mini.Threads.isRunning(): must call mini.initialize() first", -1);
	if (threadID == 0) return false;
	for (var i = 0; i < this.threads.length; ++i) {
		if (this.threads[i].id == threadID) {
			return true;
		}
	}
	return false;
};

// mini.Threads.doFrame()
// Performs update and render processing for a single frame.
// Remarks:
//     This method is meant for internal use by the threader. Calling it from
//     user code is not recommended.
mini.Threads.doFrame = function()
{
	if (!this.isInitialized)
		Abort("mini.Threads.doFrame(): must call mini.initialize() first", -1);
	if (IsMapEngineRunning()) RenderMap();
		else this.renderAll();
	FlipScreen();
	if (IsMapEngineRunning()) {
		this.hasUpdated = false;
		UpdateMapEngine();
		if (!this.hasUpdated) {
			this.updateAll();
		}
	} else {
		this.updateAll();
	}
};

// mini.Threads.join()
// Blocks until one or more threads have terminated.
// Arguments:
//     threadID: Either a single thread ID or an array of them.
// Remarks:
//     If .join() is called during an update of another thread, the blocking
//     thread will not be updated until .join() returns. However, any other threads
//     will continue to update as normal. This enables easy threading without
//     having to worry about the intricacies of cooperative threading--minithreads
//     handles it for you.
mini.Threads.join = function(threadIDs)
{
	if (!this.isInitialized)
		Abort("mini.Threads.join(): must call mini.initialize() first", -1);
	threadIDs = threadIDs instanceof Array ? threadIDs : [ threadIDs ];
	var isFinished = false;
	while (!isFinished) {
		this.doFrame();
		isFinished = true;
		for (var i = 0; i < threadIDs.length; ++i) {
			isFinished = isFinished && !this.isRunning(threadIDs[i]);
		}
	}
};

// mini.Threads.kill()
// Prematurely terminates a thread.
// Arguments:
//     threadID: The ID of the thread to terminate.
mini.Threads.kill = function(threadID)
{
	if (!this.isInitialized)
		Abort("mini.Threads.kill(): must call mini.initialize() first", -1);
	for (var i = 0; i < this.threads.length; ++i) {
		if (threadID == this.threads[i].id) {
			this.threads[i].isValid = false;
			this.threads.splice(i--, 1);
		}
	}
};

// mini.Threads.pause()
// Pauses execution of a thread.
// Arguments:
//    threadID: The ID of the thread to pause.
// Remarks:
//     While a thread is paused, its updater and input handler aren't called;
//     however, it will continue to participate in rendering.
mini.Threads.pause = function(threadID)
{
	if (!this.isInitialized)
		Abort("mini.Threads.pause(): must call mini.initialize() first", -1);
	mini.Link(this.threads).filterBy('id', threadID)
		.each(function(thread)
	{
		thread.isPaused = true;
	});
}

// mini.Threads.resume()
// Resumes execution of a paused thread. No effect on active threads.
// Arguments:
//    threadID: The ID of the thread to resume.
mini.Threads.resume = function(threadID)
{
	if (!this.isInitialized)
		Abort("mini.Threads.resume(): must call mini.initialize() first", -1);
	mini.Link(this.threads).filterBy('id', threadID)
		.each(function(thread)
	{
		thread.isPaused = false;
	});
}

// mini.Threads.self()
// Returns the currently executing thread's thread ID.
// Remarks:
//     If this function is used outside of a thread update, render or input handling
//     call, it will return 0. Zero is never a valid thread ID.
mini.Threads.self = function()
{
	return this.currentSelf;
};

// mini.Threads.renderAll()
// Renders the current frame by calling all active threads' renderers.
mini.Threads.renderAll = function()
{
	if (!this.isInitialized)
		Abort("mini.Threads.renderAll(): must call mini.initialize() first", -1);
	if (IsSkippedFrame()) return;
	mini.Link(mini.Link(this.threads).sort(this.threadSorter))
		.where(function(thread) { return thread.isValid })
		.where(function(thread) { return thread.renderer !== null })
		.each(function(thread)
	{
		this.currentSelf = thread.id;
		thread.renderer();
		this.currentSelf = 0;
	}.bind(this));
};

// mini.Threads.updateAll()
// Updates all active threads for the next frame.
mini.Threads.updateAll = function()
{
	if (!this.isInitialized)
		Abort("mini.Threads.updateAll(): must call mini.initialize() first", -1);
	var threadsEnding = [];
	mini.Link(this.threads)
		.where(function(thread) { return thread.isValid })
		.where(function(thread) { return !thread.isUpdating })
		.where(function(thread) { return !thread.isPaused })
		.each(function(thread)
	{
		this.currentSelf = thread.id;
		thread.isUpdating = true;
		var stillRunning = thread.updater();
		if (thread.inputHandler !== null && stillRunning) {
			thread.inputHandler();
		}
		thread.isUpdating = false;
		this.currentSelf = 0;
		if (!stillRunning) {
			threadsEnding.push(thread.id);
		}
	}.bind(this));
	for (var i = 0; i < threadsEnding.length; ++i) {
		this.kill(threadsEnding[i]);
	}
	this.hasUpdated = true;
};
