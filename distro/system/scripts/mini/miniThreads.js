/**
 * miniRT  (c) 2015 Fat Cerberus
 * A set of system scripts for minisphere providing advanced, high-level
 * functionality not available in the engine itself.
 *
 * [mini/miniThreads.js]
 * A cooperative threader with an API similar to pthreads, which replaces
 * Sphere's update and render scripts with a much more robust solution.
**/

if (typeof mini === 'undefined') {
    Abort("miniRT component script; use miniRT.js instead", -2);
}

RequireSystemScript('mini/miniLink.js');

// Threads object
// Encapsulates the thread manager.
mini.Threads = new (function()
{
	this.isInitialized = false;
})();

// initializer registration
// Initializes Threads when the user calls mini.initialize().
mini.onStartUp.add(mini.Threads, function(params)
{
	Print("miniRT: Initializing miniThreads");
	
	try {
		this.isInitialized = true;
		this.currentSelf = 0;
		this.hasUpdated = false;
		this.nextThreadID = 1;
		this.threads = [];
		
		this.threadSorter = function(a, b) {
			return a.priority != b.priority ?
				a.priority - b.priority :
				a.id - b.id;
		};
		
		SetUpdateScript(mini.Threads.updateAll.bind(mini.Threads));
		SetRenderScript(mini.Threads.renderAll.bind(mini.Threads));
	}
	catch(err) {
		this.isInitialized = false;
		throw err;
	}
});

// mini.Threads.updateAll()
// Updates all active threads for the next frame.
mini.Threads.updateAll = function(threadID)
{
	Assert(this.isInitialized, "mini.initialize() must be called first", -1);
	
	var threadsEnding = [];
	mini.Link(mini.Link(this.threads).toArray())
		.where(function(thread) { return thread.isValid; })
		.where(function(thread) { return !thread.isBusy; })
		.each(function(thread)
	{
		var lastSelf = this.currentSelf;
		thread.isBusy = true;
		this.currentSelf = thread.id;
		var isRunning = thread.updater(thread.id);
		if (thread.inputHandler !== undefined && isRunning)
			thread.inputHandler();
		this.currentSelf = lastSelf;
		thread.isBusy = false;
		if (!isRunning) threadsEnding.push(thread.id);
	}.bind(this));
	mini.Link(threadsEnding)
		.each(function(threadID)
	{
		mini.Threads.kill(threadID);
	});
	this.hasUpdated = true;
}

// mini.Threads.renderAll()
// Renders the current frame by calling all active threads' renderers.
mini.Threads.renderAll = function()
{
	Assert(this.isInitialized, "mini.initialize() must be called first!", -1);
	
	if (IsSkippedFrame()) return;
	mini.Link(mini.Link(this.threads).sort(this.threadSorter))
		.where(function(thread) { return thread.isValid; })
		.where(function(thread) { return thread.renderer !== undefined; })
		.each(function(thread)
	{
		thread.renderer();
	}.bind(this));
};

// mini.Threads.create()
// Creates an object thread. This is the recommended thread creation method.
// Arguments:
//     entity:   The object for which to create the thread. This object's .update() method
//               will be called once per frame, along with .render() and .getInput() if they
//               exist, until .update() returns false.
//     priority: Optional. The render priority for the new thread. Higher-priority threads are rendered
//               later in a frame than lower-priority ones. Ignored if no renderer is provided. (default: 0)
mini.Threads.create = function(entity, priority)
{
	Assert(this.isInitialized, "mini.initialize() must be called first", -1);
	Assert(entity instanceof Object || entity === null, "create() argument must be a valid object", -1);
	
	priority = priority !== undefined ? priority : 0;
	
	var update = entity.update;
	var render = (typeof entity.render === 'function') ? entity.render : undefined;
	var getInput = (typeof entity.getInput === 'function') ? entity.getInput : null;
	return this.createEx(entity, {
		priority: priority,
		update: entity.update,
		render: entity.render,
		getInput: entity.getInput,
	});
};

// mini.Threads.createEx()
// Creates a thread with advanced options.
// Arguments:
//     that:       The object to bind as 'this' to thread callbacks. May be null.
//     threadDesc: An object describing the thread. This should contain the following members:
//                     update:   The update function for the new thread.
//                     render:   Optional. The render function for the new thread.
//                     getInput: Optional. The input handler for the new thread.
//                     priority: Optional. The render priority for the new thread. Higher-priority threads
//                               are rendered later in a frame than lower-priority ones. Ignored if no
//                               renderer is provided. (default: 0)
// Remarks:
//     This is for advanced thread creation. For typical use, it is recommended to use
//     Threads.create() or Threads.doWith() instead.
mini.Threads.createEx = function(that, threadDesc)
{
	Assert(this.isInitialized, "mini.initialize() must be called first", -1);
	Assert(arguments.length >= 2, "mini.Threads.createEx() expects 2 arguments", -1);
	
	var update = threadDesc.update.bind(that);
	var render = typeof threadDesc.render === 'function'
		? threadDesc.render.bind(that) : undefined;
	var getInput = typeof threadDesc.getInput === 'function'
		? threadDesc.getInput.bind(that) : undefined;
	var priority = 'priority' in threadDesc ? threadDesc.priority : 0;
	var newThread = {
		isValid: true,
		id: this.nextThreadID++,
		that: that,
		inputHandler: getInput,
		isBusy: false,
		priority: priority,
		renderer: render,
		updater: update,
	};
	var startTime = GetSeconds();
	this.threads.push(newThread);
	return newThread.id;
};

// .isRunning() method
// Determines whether a thread is still running.
// Arguments:
//     threadID: The ID of the thread to check.
mini.Threads.isRunning = function(threadID)
{
	Assert(this.isInitialized, "mini.initialize() must be called first", -1);
	
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
	Assert(this.isInitialized, "mini.initialize() must be called first", -1);
	
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
// Blocks the calling thread until one or more other threads have terminated.
// Arguments:
//     threadID: Either a single thread ID or an array of them. Any invalid thread
//               ID will cause an error to be thrown.
mini.Threads.join = function(threadIDs)
{
	Assert(this.isInitialized, "mini.initialize() must be called first", -1);
	
	threadIDs = threadIDs instanceof Array ? threadIDs : [ threadIDs ];
	while (mini.Link(this.threads)
		.filterBy('id', threadIDs)
		.length() > 0)
	{
		this.doFrame();
	}
};

// mini.Threads.kill()
// Forcibly terminates a thread.
// Arguments:
//     threadID: The ID of the thread to terminate.
mini.Threads.kill = function(threadID)
{
	Assert(this.isInitialized, "mini.initialize() must be called first", -1);
	
	mini.Link(this.threads)
		.where(function(thread) { return thread.id == threadID })
		.execute(function(thread) { thread.isValid = false; })
		.remove();
};

// mini.Threads.self()
// Returns the currently executing thread's thread ID.
// Remarks:
//     If this function is used outside of a thread update, render or input handling
//     call, it will return 0 (the ID of the main thread).
mini.Threads.self = function()
{
	Assert(this.isInitialized, "mini.initialize() must be called first", -1);
	
	return this.currentSelf;
};
