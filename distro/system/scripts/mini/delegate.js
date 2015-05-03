/**
 * minidelegate 1.1b3 - (c) 2015 Bruce Pascoe
 * A very simple and easy-to-use multicast delegate implementation.
**/

// MultiDelegate() constructor
// Constructs a multicast delegate object.
function MultiDelegate()
{
	this.invokeList = [];
}

// .add() method
// Adds a method to the delegate's invocation list.
// Arguments:
//     o:      The object which will be bound to 'this' when the
//             delegate is invoked.
//     method: The method to be called.
MultiDelegate.prototype.add = function(o, method)
{
	this.invokeList.push({ o:o, method:method });
};

// .remove() method
// Removes a method that was previously added with .add().
// Remarks:
//     Takes the same arguments, with the same semantics, as .add().
//     .add() must already have been called with the same arguments.
MultiDelegate.prototype.remove = function(o, method)
{
	for (var i = 0; i < this.invokeList.length; ++i) {
		if (o == this.invokeList[i].o && method == this.invokeList[i].method) {
			this.invokeList.splice(i, 1);
			break;
		}
	}
};

// .invoke() method
// Calls all methods in the invocation list and returns the result of
// the last method called.
MultiDelegate.prototype.invoke = function()
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
