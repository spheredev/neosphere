/**
 * minisphere Runtime 1.1 - (c) 2015 Fat Cerberus
 * A set of system scripts providing advanced, high-level functionality not
 * available in the engine itself.
 *
 * [mini/miniPact.js]
 * A promise implementation for minisphere, based on the Promises/A+
 * specification. The implementation is fully compliant.
**/

if (typeof mini === 'undefined') {
    Abort("miniRT component script; use miniRT.js instead", -2);
}

mini.Promise = (function(undefined)
{
	'use strict';
	
	function Promise(fn)
	{
		var deferred = [];
		var state = 'pending';
		var result = undefined;
		var self = this;
		
		function handle(handler)
		{
			if (state == 'pending')
				deferred.push(handler);
			else Async(function() {
				var callback = state == 'fulfilled' ? handler.fulfiller
					: state == 'rejected' ? handler.rejector
					: undefined;
				if (typeof callback !== 'function') {
					if (state == 'fulfilled') handler.resolve(result);
					if (state == 'rejected') handler.reject(result);
				} else {
					handler.resolve(callback(result));
				}
			});
		}
		
		function resolve(value)
		{
			if (value === self) {
				reject(new TypeError("Attempt to fulfill a promise with itself"));
				return;
			} else if (state != 'pending')
				return;
			try {
				if ((typeof value === 'function' || typeof value === 'object')
					&& value !== null)
				{
					var then = value.then;
					if (typeof then === 'function') {
						then.call(value, resolve, reject);
						return;
					}
				}
				state = 'fulfilled';
				result = value;
				for (var i = 0; i < deferred.length; ++i)
					handle(deferred[i]);
				deferred = [];
			} catch(e) {
				reject(e);
			}
		}
		
		function reject(reason)
		{
			if (state != 'pending') return;
			state = 'rejected'
			result = reason;
			for (var i = 0; i < deferred.length; ++i)
				handle(deferred[i]);
			deferred = [];
		}
		
		this.toString = function()
		{
			return state != 'pending'
				? "[promise: " + state + " `" + result + "`]"
				: "[promise: pending]";
		}
		
		this.catch = function(errback)
		{
			return this.then(undefined, errback);
		};
		
		this.then = function(callback, errback)
		{
			var promise = this;
			return new mini.Promise(function(resolve, reject) {
				handle({
					promise: promise,
					resolve: resolve, reject: reject,
					fulfiller: callback,
					rejector: errback
				});
			});
		};
		
		this.done = function(callback, errback)
		{
			var self = arguments.length > 0 ? this.then.apply(this, arguments) : this;
			if (typeof errback !== 'function')
				self.catch(function(reason) { throw reason; });
		};
		
		try {
			fn.call(this, resolve, reject);
		} catch(e) {
			reject(e);
		}
	};
	
	Promise.all = function(iterable)
	{
		return new mini.Promise(function(resolve, reject) {
			var promises = [];
			var values = [];
			var numPromises = iterable.length;
			if (numPromises == 0)
				resolve([]);
			else {
				for (var i = 0; i < numPromises; ++i) {
					var v = iterable[i];
					if (!v || typeof v.then !== 'function')
						v = Promise.resolve(v);
					promises.push(v);
					v.then(function(value) {
						values.push(value);
						if (values.length == numPromises)
							resolve(values);
					}, function(reason) {
						reject(reason);
					});
				}
			}
		});
	};
	
	Promise.race = function(iterable)
	{
		return new mini.Promise(function(resolve, reject) {
			var numPromises = iterable.length;
			for (var i = 0; i < numPromises; ++i) {
				var v = iterable[i];
				if (!v || typeof v.then !== 'function')
					v = Promise.resolve(v);
				v.then(function(value) { resolve(value); },
					function(reason) { reject(reason); });
			}
		});
	};
	
	Promise.reject = function(reason)
	{
		return new mini.Promise(function(resolve, reject) {
			reject(reason);
		});
	};
	
	Promise.resolve = function(value)
	{
		if (value instanceof Promise) return value;
		return new mini.Promise(function(resolve, reject) {
			resolve(value);
		});
	};
	
	return Promise;
})();

mini.Pact = (function(undefined)
{
	'use strict';
	
	function Pact()
	{
		var numPending = 0;
		var handlers = [];
		
		// checkPromise() [internal]
		// Checks if the specified promise object came from this pact. If not,
		// aborts with an error.
		// Arguments:
		//     promise: The promise to check.
		// Returns:
		//     The promise handler.
		function checkPromise(promise)
		{
			if (!(promise instanceof mini.Promise))
				Abort("argument is not a promise", -2);
			for (var i = handlers.length - 1; i >= 0; --i)
				if (handlers[i].that == promise) return handlers[i];
			Abort("promise was not made from this pact", -2);
		};
		
		// mini.Pact:makePromise()
		// Makes a new promise with this pact.
		// Arguments: None.
		this.makePromise = function()
		{
			++numPending;
			var handler;
			var promise = new mini.Promise(function(resolve, reject) {
				handler = { resolve: resolve, reject: reject };
			})
			promise.then(
				function(value) { --numPending; },
				function(reason) { --numPending; }
			);
			handler.that = promise;
			handlers.push(handler);
			return promise;
		};
		
		// mini.Pact:resolve()
		// Resolves a promise originating from this pact.
		// Arguments:
		//     promise: The promise to resolve. If the promise wasn't made from this pact,
		//              a TypeError will be thrown.
		//     value:   The value with which to resolve the promise.
		this.resolve = function(promise, value)
		{
			checkPromise(promise).resolve(value);
		};
		
		// mini.Pact:reject()
		// Rejects a promise originating from this pact.
		// Arguments:
		//     promise: The promise to reject. If the promise wasn't made from this pact,
		//              a TypeError will be thrown.
		//     reason:  The value to reject with (usually an Error object).
		this.reject = function(promise, reason)
		{
			checkPromise(promise).reject(reason);
		};
		
		// mini.Pact:welch()
		// Rejects all outstanding promises from this pact.
		// Arguments:
		//     reason: The value to reject with (usually an Error object).
		this.welch = function(reason)
		{
			for (var i = handlers.length - 1; i >= 0; --i)
				handlers[i].reject(reason);
		};
		
		this.toString = function()
		{
			return "[pact: " + numPending.toString() + " outstanding]";
		};
	}
	
	return Pact;
})();
