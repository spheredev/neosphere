/**
 *  miniRT/pacts CommonJS module
 *  easy-to-use promises for Sphere, based on Promises/A+
 *  (c) 2015-2016 Fat Cerberus
**/

if (typeof exports === 'undefined')
{
    throw new TypeError("script must be loaded with require()");
}

var pacts =
module.exports = (function()
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
			return new Promise(function(resolve, reject) {
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
		return new Promise(function(resolve, reject) {
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
		return new Promise(function(resolve, reject) {
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
		return new Promise(function(resolve, reject) {
			reject(reason);
		});
	};

	Promise.resolve = function(value)
	{
		if (value instanceof Promise) return value;
		return new Promise(function(resolve, reject) {
			resolve(value);
		});
	};

	function Pact()
	{
		var numPending = 0;
		var handlers = [];

		function checkPromise(promise)
		{
			if (!(promise instanceof Promise))
				Abort("argument is not a promise", -2);
			for (var i = handlers.length - 1; i >= 0; --i)
				if (handlers[i].that == promise) return handlers[i];
			Abort("promise was not made from this pact", -2);
		};

		// Pact:promise()
		// makes a new promise with this pact.
		this.promise = function()
		{
			++numPending;
			var handler;
			var promise = new Promise(function(resolve, reject) {
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

		// Pact:resolve()
		// resolve a promise originating from this pact.
		// arguments:
		//     promise: the promise to resolve.  if the promise wasn't made from this pact,
		//              a TypeError will be thrown.
		//     value:   the value with which to resolve the promise.
		this.resolve = function(promise, value)
		{
			checkPromise(promise).resolve(value);
		};

		// Pact:reject()
		// reject a promise originating from this pact.
		// arguments:
		//     promise: the promise to reject.  if the promise wasn't made from this pact,
		//              a TypeError will be thrown.
		//     reason:  the value to reject with (usually an Error object).
		this.reject = function(promise, reason)
		{
			checkPromise(promise).reject(reason);
		};

		// Pact:welsh()
		// reject all outstanding promises from this pact.
		// arguments:
		//     reason: the value to reject with (usually an Error object).
		this.welsh = function(reason)
		{
			for (var i = handlers.length - 1; i >= 0; --i)
				handlers[i].reject(reason);
		};

		this.toString = function()
		{
			return "[pact: " + numPending.toString() + " outstanding]";
		};
	}

	return {
		Pact:    Pact,
		Promise: Promise,
	}
})();
