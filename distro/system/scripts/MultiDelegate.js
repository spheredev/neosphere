/**
 * MultiDelegate for JavaScript - (c) 2012-2013 Bruce Pascoe
 * A simple multicast delegate implementation for JavaScript.
**/

var MultiDelegate = MultiDelegate || {};

// MultiDelegate() constructor
// Creates an object representing a multicast delegate.
function MultiDelegate()
{
	this.invocationList = [];
}

// .addHook() method
// Adds a method to the delegate's invocation list.
// Arguments:
//     o      - The object to pass as 'this' to the specified method.
//     method - The method to be called.
MultiDelegate.prototype.addHook = function(o, method)
{
	this.invocationList.push({ o:o, method:method });
};

// .invoke() method
// Calls all methods in the invocation list and returns the result of the last method called.
MultiDelegate.prototype.invoke = function()
{
	var result = undefined;
	for (var i = 0; i < this.invocationList.length; ++i) {
		var o = this.invocationList[i].o;
		var method = this.invocationList[i].method;
		result = method.apply(o, arguments);
	}
	// Return result of last method call
	return result;
};

// .removeHook() method
// Removes a method that was previously added with addHook().
// Remarks:
//     Takes the same arguments, with the same semantics, as addHook(). addHook() must already have been
//     called with the same arguments.
MultiDelegate.prototype.removeHook = function(o, method)
{
	for (var i = 0; i < this.invocationList.length; ++i) {
		if (o == this.invocationList[i].o && method == this.invocationList[i].method) {
			this.invocationList.splice(i, 1);
			break;
		}
	}
};
