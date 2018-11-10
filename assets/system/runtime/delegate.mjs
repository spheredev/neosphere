/**
 *  Sphere Runtime for Sphere games
 *  Copyright (c) 2015-2018, Fat Cerberus
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

const kInvokeList = Symbol("invocation list");

export default
class Delegate
{
	get [Symbol.toStringTag]() { return 'Delegate'; }

	constructor()
	{
		this[kInvokeList] = [];
	}

	addHandler(handler, thisObj)
	{
		if (haveHandler(this, handler, thisObj))
			throw new Error("Cannot add the same handler more than once");
		this[kInvokeList].push({ thisObj, handler });
	}

	call(...args)
	{
		let lastResult = undefined;
		for (const entry of this[kInvokeList])
			lastResult = entry.handler.apply(entry.thisObj, args);

		// use the return value of the last handler called
		return lastResult;
	}

	removeHandler(handler, thisObj)
	{
		if (!haveHandler(this, handler, thisObj))
			throw new Error("Handler is not registered");
		this[kInvokeList] = this[kInvokeList]
			.filter(it => it.handler !== handler || it.thisObj !== thisObj)
	}

}

function haveHandler(delegate, handler, thisObj)
{
	return delegate[kInvokeList]
		.some(it => it.handler === handler && it.thisObj === thisObj);
}
