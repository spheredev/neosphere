/**
 *  miniRT events CommonJS module
 *  (c) 2015-2016 Fat Cerberus
**/

'use strict';
exports.__esModule = true;
exports.default = exports;

const from = require('from');

exports.Delegate = Delegate;
function Delegate()
{
    this.invokeList = [];
}

Delegate.prototype.add = function add(handler, thisObj)
{
	if (_haveHandler(this, handler, thisObj))
		throw new Error("duplicate handler");
	this.invokeList.push({ thisObj: thisObj, handler: handler });
}

Delegate.prototype.call = function call(/*...*/)
{
	var lastResult = undefined;
	var invokeArgs = arguments;
	from.Array(this.invokeList).each(function(v) {
		lastResult = v.handler.apply(v.thisObj, invokeArgs);
	});

	// use the return value of the last handler called
	return lastResult;
}

Delegate.prototype.remove = function remove(handler, thisObj)
{
	if (!_haveHandler(this, handler, thisObj))
		throw new Error("handler is not registered");
	from.Array(this.invokeList)
		.where(function(v) { return v.handler == handler; })
		.where(function(v) { return v.thisObj == thisObj; })
		.remove();
}

function _haveHandler(delegate, handler, thisObj)
{
	return from.Array(delegate.invokeList).any(function(v) {
		return v.handler == handler && v.thisObj == thisObj;
	});
}
