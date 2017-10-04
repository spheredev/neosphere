/**
 *  Sphere Runtime for Sphere games
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

import { from, Pact } from 'sphere-runtime';

let inputThreads = [],
	threadPact = new Pact(),
    threads = [];

export default
class Thread
{
	get [Symbol.toStringTag]() { return 'Thread'; }

	static join(thread)
	{
		return thread._promise;
	}

	constructor(options = {})
	{
		if (new.target === Thread)
			throw new Error(`'${new.target.name}' is abstract and cannot be instantiated`);

		options = Object.assign({}, {
			priority: 0,
		}, options);

		this._busy = false;
		this._terminating = false;
		this._priority = options.priority;
		this._promise = threadPact.makePromise();
		this._renderJob = null;
		this._updateJob = null;

		threads.push(this);
	}

	get hasFocus()
	{
		return this === inputThreads[inputThreads.length - 1];
	}

	async on_inputCheck() {}
	on_render() {}
	async on_update() {}

	dispose()
	{
		this.stop();
		this._terminating = true;
	}

	start()
	{
		this._started = true;
		this._renderJob = Dispatch.onRender(() => {
			this.on_render();
		}, this._priority);
		this._updateJob = Dispatch.onUpdate(async () => {
			if (this._busy)
				return;
			this._busy = true;
			if (this === inputThreads[inputThreads.length - 1])
				await this.on_inputCheck();
			await this.on_update();
			this._busy = false;
		}, this._priority);
	}

	stop()
	{
		this.yieldInput();
		Dispatch.cancel(this._updateJob);
		Dispatch.cancel(this._renderJob);
		this._started = false;
	}

	takeInput()
	{
		if (this.on_inputCheck === Thread.on_inputCheck)
			throw new TypeError("cannot take input without an input handler");

		inputThreads = from.Array(inputThreads)
			.where(it => it != this)
			.including([ this ])
			.toArray();
	}

	yieldInput()
	{
		if (this === inputThreads[inputThreads.length - 1])
			inputThreads.pop();
	}
}
