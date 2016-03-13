/**
 *  miniRT/delegates 2.0 CommonJS module
 *  (c) 2015-2016 Fat Cerberus
 *  a multicast delegate implementation inspired by C# events.
**/

if (typeof exports === 'undefined')
{
    throw new TypeError("delegates.js must be loaded using require()");
}

var delegates =
module.exports = (function()
{
	return {
		Delegate: Delegate,
	};
	
	function Delegate()
	{
		var invokeList = [];

		return {
			add:    add,
			invoke: invoke,
			remove: remove,
		};

		// delegate:add()
		// add a method to the delegate's invocation list.
		// arguments:
		//     o:      the object which will be bound to 'this' when the
		//             delegate is invoked.
		//     method: the method to be called.
		function add(o, method)
		{
			invokeList.push({ o:o, method:method });
		};

		// delegate:remove()
		// remove a method that was previously added with .add().
		// remarks:
		//     takes the same arguments, with the same semantics, as delegate:add().
		//     delegate:add() must already have been called with the same arguments.
		function remove(o, method)
		{
			for (var i = invokeList.length - 1; i >= 0; --i) {
				if (o == invokeList[i].o && method == invokeList[i].method) {
					invokeList.splice(i, 1);
					break;
				}
			}
		};

		// delegate:invoke()
		// call all methods in the delegate's invocation list, returning
		// the result of the last method called.
		function invoke()
		{
			var result = undefined;
			for (var i = 0; i < invokeList.length; ++i) {
				var o = invokeList[i].o;
				var method = invokeList[i].method;
				result = method.apply(o, arguments);
			}
			
			// Return result of last method call
			return result;
		};
	}
})();
