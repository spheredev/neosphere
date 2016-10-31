'use strict';
if (global.Promise === undefined)
	global.Promise = Promise;

function Promise(executor)
{
	if (!(this instanceof Promise))
		throw new TypeError("constructor requires 'new'");
	if (typeof executor !== 'function')
		throw new TypeError("executor is not callable");
	
	var deferred = [];
	var state = 'pending';
	var result = undefined;
	var self = this;

	function handle(handler)
	{
		if (state == 'pending')
			deferred.push(handler);
		else system.defer(function() {
			var callback = state == 'fulfilled' ? handler.fulfiller
				: state == 'rejected' ? handler.rejector
				: undefined;
			if (typeof callback !== 'function') {
				if (state == 'fulfilled') handler.resolve(result);
				if (state == 'rejected') handler.reject(result);
			}
			else {
				handler.resolve(callback(result));
			}
		});
	}

	function resolve(value)
	{
		if (value === self) {
			reject(new TypeError("cannot fulfill promise with itself"));
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
		}
		catch (e) {
			reject(e);
		}
	}

	function reject(reason)
	{
		if (state !== 'pending')
			return;
		state = 'rejected';
		result = reason;
		for (var i = 0; i < deferred.length; ++i)
			handle(deferred[i]);
		deferred = [];
	}

	this.toString = function()
	{
		return "[object Promise]";
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
		if (typeof errback !== 'function') {
			self.catch(function(reason) {
				system.dispatch(function() { throw reason; });
			});
		}
	};

	try {
		executor.call(this, resolve, reject);
	} catch(e) {
		reject(e);
	}
};

Promise.all = function all(iterable)
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
				}, reject);
			}
		}
	});
};

Promise.race = function race(iterable)
{
	return new Promise(function(resolve, reject) {
		var numPromises = iterable.length;
		for (var i = 0; i < numPromises; ++i) {
			var v = iterable[i];
			if (!v || typeof v.then !== 'function')
				v = Promise.resolve(v);
			v.then(resolve, reject);
		}
	});
};

Promise.reject = function reject(reason)
{
	return new Promise(function(resolve, reject) {
		reject(reason);
	});
};

Promise.resolve = function resolve(value)
{
	if (value instanceof Promise) return value;
	return new Promise(function(resolve, reject) {
		resolve(value);
	});
};
