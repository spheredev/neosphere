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

import from from 'from';
import Pact from 'pact';

let currentSelf = null,
    inputThreads = [],
    threadPact = new Pact();

export default
class Thread
{
	get [Symbol.toStringTag]() { return 'Thread'; }

	static create(entity, priority)
	{
		let thread = new PromptThread(entity, priority);
		thread.start();
		return thread;
	}

	static join(...threads)
	{
		let promises = from(threads)
			.select(it => it._promise);
		return Promise.all(promises);
	}

	static self()
	{
		return currentSelf;
	}

	constructor(options = {})
	{
		if (new.target === Thread)
			throw new Error(`'${new.target.name}' is abstract and cannot be instantiated`);

		options = Object.assign({}, {
			priority: 0,
		}, options);

		this._started = false;
		this._busy = false;
		this._priority = options.priority;
		this._promise = Promise.resolve();
		this._renderJob = null;
		this._updateJob = null;
	}

	get hasFocus()
	{
		return this === inputThreads[inputThreads.length - 1];
	}

	get running()
	{
		return this._started;
	}

	on_inputCheck() {}
	on_render() {}
	on_update() {}

	start()
	{
		if (this._started)
			return;

		this._started = true;
		this._promise = threadPact.makePromise();
		this._renderJob = Dispatch.onRender(() => {
			this.on_render();
		}, this._priority);
		this._updateJob = Dispatch.onUpdate(async () => {
			if (this._busy)
				return;
			let lastSelf = currentSelf;
			currentSelf = this;
			this._busy = true;
			if (this === inputThreads[inputThreads.length - 1])
				await this.on_inputCheck();
			await this.on_update();
			this._busy = false;
			currentSelf = lastSelf;
		}, this._priority);

		// after thread terminates, remove from input stack
		this._promise.then(() => {
			from.Array(inputThreads)
				.where(it => !it._started)
				.remove();
		});
	}

	stop()
	{
		if (!this._started)
			return;

		this.yieldInput();
		this._updateJob.cancel();
		this._renderJob.cancel();
		this._started = false;
		threadPact.resolve(this._promise);
	}

	takeInput()
	{
		if (this.on_inputCheck === Thread.on_inputCheck)
			throw new TypeError("thread doesn't accept user input");

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

class PromptThread extends Thread
{
	constructor(entity, priority = 0)
	{
		super({ priority });

		this.entity = entity;
	}

	async on_checkInput()
	{
		if (this.entity.inputChecker !== undefined)
			await this.entity.inputChecker();
	}

	on_render()
	{
		if (this.entity.renderer !== undefined)
			return this.entity.renderer();
	}

	async on_update()
	{
		if (this.entity.updater === undefined || !await this.entity.updater())
			this.stop();
	}
}
