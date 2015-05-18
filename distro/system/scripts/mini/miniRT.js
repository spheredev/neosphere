/**
 * minisphere Runtime 1.1 - (c) 2015 Fat Cerberus
 * A set of system scripts providing advanced, high-level functionality not
 * available in the engine itself.
 *
 * [mini/miniRT.js]
 * The main script for the minisphere Runtime script. Requiring this script
 * is sufficient to use any component of the runtime.
**/

// mini object
// Encapsulates the entire minisphere Runtime.
mini = typeof mini !== 'undefined' ? mini :
new (function() {
	this.initializers = [];
})();

// mini.Delegate()
// Constructs a multicast delegate.
mini.Delegate = function()
{
	this.invokeList = [];
}

// mini.Delegate:add()
// Adds a method to the delegate's invocation list.
// Arguments:
//     o:      The object which will be bound to 'this' when the
//             delegate is invoked.
//     method: The method to be called.
mini.Delegate.prototype.add = function(o, method)
{
	this.invokeList.push({ o:o, method:method });
};

// mini.Delegate:remove()
// Removes a method that was previously added with .add().
// Remarks:
//     Takes the same arguments, with the same semantics, as .add().
//     .add() must already have been called with the same arguments.
mini.Delegate.prototype.remove = function(o, method)
{
	for (var i = this.invokeList.length - 1; i >= 0; --i) {
		if (o == this.invokeList[i].o && method == this.invokeList[i].method) {
			this.invokeList.splice(i, 1);
			break;
		}
	}
};

// mini.Delegate:invoke()
// Calls all methods in the delegate's invocation list and returns
// the result of the last method called.
mini.Delegate.prototype.invoke = function()
{
	var result = undefined;
	for (var i = 0; i < this.invokeList.length; ++i) {
		var o = this.invokeList[i].o;
		var method = this.invokeList[i].method;
		result = method.apply(o, arguments);
	}
	
	// Return result of last method call
	return result;
};

// mini.initialize()
// Initializes miniRT and all registered components.
// Arguments:
//     params: An object specifying initialization parameters. This object will
//             be passed to each registered initializer.
mini.initialize = function(params)
{
    params = typeof params !== 'undefined' ? params : {};
    
	Print("miniRT: Initializing miniRT core");
	var frameRate = 'frameRate' in params ? params.frameRate : 0;
	SetFrameRate(frameRate);
    mini.onStartUp.invoke(params);
}

// mini.onStartUp
// Invoked when the runtime is initialized. This is used by runtime
// component scripts to provide encapsulation.
// Arguments:
//     params: The `params` object which was passed to mini.initialize().
mini.onStartUp = new mini.Delegate();

RequireSystemScript('mini/miniLink.js');
RequireSystemScript('mini/miniBGM.js');
RequireSystemScript('mini/miniConsole.js');
RequireSystemScript('mini/miniPact.js');
RequireSystemScript('mini/miniScenes.js');
RequireSystemScript('mini/miniThreads.js');
