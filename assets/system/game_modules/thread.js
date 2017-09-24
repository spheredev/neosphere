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

'use strict';
const from = require('from');

let currentSelf = 0,
    haveMapEngine = typeof MapEngine === 'function',
    initialized = false,
    nextThreadID = 1,
    threads = [];

class Thread
{
	static create(entity, priority = 0)
	{
		if (!initialized) {
			Dispatch.onUpdate(updateAllThreads);
			Dispatch.onRender(renderAllThreads);
			initialized = true;
		}
		return makeThread(entity, {
			priority: priority,
			update: entity.update,
			render: entity.render,
			getInput: entity.getInput,
		});
	}

	static isRunning(threadID)
	{
		if (threadID == 0)
			return false;
		for (var i = 0; i < threads.length; ++i) {
			if (threads[i].id == threadID)
				return true;
		}
		return false;
	}

	static join(threadIDs)
	{
		threadIDs = threadIDs instanceof Array ? threadIDs : [ threadIDs ];
		while (from.Array(threads)
			.where(it => threadIDs.indexOf(it.id) >= 0)
			.count() > 0)
		{
			if (haveMapEngine && IsMapEngineRunning()) {
				UpdateMapEngine();
				RenderMap();
			}
			screen.flip();
		}
	}

	static kill(threadID)
	{
		from.Array(threads)
			.where(it => it.id == threadID)
			.besides(it => it.isValid = false)
			.remove();
	}

	static self()
	{
		return currentSelf;
	}

	constructor(options = {})
	{
		if (new.target === Thread)
			throw new TypeError(`'${new.target.name}' cannot be instantiated directly`);

		this.threadID = null;
		this.threadPriority = options.priority !== undefined ? options.priority : 0;
	}

	on_checkInput() {}
	on_render() {}
	on_update() {}

	get running()
	{
		return Thread.isRunning(this.threadID);
	}

	dispose()
	{
		this.stop();
	}

	join()
	{
		Thread.join(this.threadID);
	}

	start()
	{
		this.threadID = Thread.create({
			getInput: () => { this.on_checkInput(); },
			update:   () => { this.on_update(); return true; },
			render:   () => { this.on_render(); },
		}, this.threadPriority);
	}

	stop()
	{
		kill(this.threadID);
	}
}

function makeThread(that, threadDesc)
{
	var update = threadDesc.update.bind(that);
	var render = typeof threadDesc.render === 'function'
		? threadDesc.render.bind(that) : undefined;
	var getInput = typeof threadDesc.getInput === 'function'
		? threadDesc.getInput.bind(that) : undefined;
	var priority = 'priority' in threadDesc ? threadDesc.priority : 0;
	var newThread = {
		isValid: true,
		id: nextThreadID++,
		that: that,
		inputHandler: getInput,
		isBusy: false,
		priority: priority,
		renderer: render,
		updater: update,
	};
	threads.push(newThread);
	threads.sort((a, b) => {
		return a.priority != b.priority ? a.priority - b.priority
			: a.id - b.id;
	});
	return newThread.id;
}

function renderAllThreads()
{
	let activeThreads = from.Array(threads.slice())
		.where(it => it.isValid)
		.where(it => it.renderer !== undefined)
    for (let thread of activeThreads)
		thread.renderer();
}

function updateAllThreads()
{
	let activeThreads = from.Array(threads.slice())
		.where(it => it.isValid && !it.isBusy)
	let threadsEnding = [];
    for (let thread of activeThreads) {
		let lastSelf = currentSelf;
		thread.isBusy = true;
		currentSelf = thread.id;
		let isRunning = thread.updater(thread.id);
		if (thread.inputHandler !== undefined && isRunning)
			thread.inputHandler();
		currentSelf = lastSelf;
		thread.isBusy = false;
		if (!isRunning)
			threadsEnding.push(thread.id);
	}
	for (let threadID of threadsEnding)
        Thread.kill(threadID);
}

// CommonJS
exports = module.exports = Thread;
Object.assign(exports, {
	__esModule: true,
	Thread:     Thread,
	default:    Thread,
});
