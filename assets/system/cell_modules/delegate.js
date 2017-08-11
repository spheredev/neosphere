/**
 *  Sphere Runtime for Cellscripts
 *  Copyright (c) 2015-2017, Fat Cerberus
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither the name of miniSphere nor the names of its contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
**/

'use strict';
exports = module.exports = Delegate;
exports.__esModule = true;
exports.default = exports;

const from = require('from');

function Delegate()
{
    this._invokeList = [];
}

Delegate.prototype.add = function add(handler, thisObj)
{
	if (haveHandler(this, handler, thisObj))
		throw new Error("duplicate handler");
	this._invokeList.push({ thisObj: thisObj, handler: handler });
}

Delegate.prototype.call = function call(/*...*/)
{
	var lastResult = undefined;
	var invokeArgs = arguments;
	from.Array(this._invokeList).each(function(v) {
		lastResult = v.handler.apply(v.thisObj, invokeArgs);
	});

	// use the return value of the last handler called
	return lastResult;
}

Delegate.prototype.remove = function remove(handler, thisObj)
{
	if (!haveHandler(this, handler, thisObj))
		throw new Error("handler is not registered");
	from.Array(this._invokeList)
		.where(function(v) { return v.handler == handler; })
		.where(function(v) { return v.thisObj == thisObj; })
		.remove();
}

function haveHandler(delegate, handler, thisObj)
{
	return from.Array(delegate._invokeList).any(function(v) {
		return v.handler == handler && v.thisObj == thisObj;
	});
}
