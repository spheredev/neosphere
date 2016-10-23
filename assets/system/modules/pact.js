/**
 *  miniRT pact CommonJS module
 *  (c) 2015-2016 Fat Cerberus
**/

'use strict';
module.exports =
{
	Pact: Pact
};

function Pact()
{
	var numPending = 0;
	var handlers = [];

	function checkPromise(promise)
	{
		if (!(promise instanceof Promise))
			throw new TypeError("argument is not a promise");
		for (var i = handlers.length - 1; i >= 0; --i)
			if (handlers[i].that == promise) return handlers[i];
		throw new TypeError("unrecognized promise");
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
		return "[object Pact]";
	};
}
