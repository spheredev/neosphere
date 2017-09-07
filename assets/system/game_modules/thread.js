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
exports = module.exports = Thread;
exports.__esModule = true;
exports.default = exports;

const from = require('from');

var currentSelf = 0;
var haveMapEngine = typeof MapEngine === 'function';
var nextThreadID = 1;
var threads = [];
Dispatch.onUpdate(_updateAll);
Dispatch.onRender(_renderAll);

Thread.create = function create(entity, priority)
{
	priority = priority !== undefined ? priority : 0;

	var update = entity.update;
	var render = (typeof entity.render === 'function') ? entity.render : undefined;
	var getInput = (typeof entity.getInput === 'function') ? entity.getInput : null;
	return _makeThread(entity, {
		priority: priority,
		update: entity.update,
		render: entity.render,
		getInput: entity.getInput,
	});
}

Thread.isRunning = function isRunning(threadID)
{
	if (threadID == 0)
		return false;
	for (var i = 0; i < threads.length; ++i) {
		if (threads[i].id == threadID)
			return true;
	}
	return false;
}

Thread.join = function join(threadIDs)
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

Thread.kill = function kill(threadID)
{
	from.Array(threads)
		.where(it => it.id == threadID)
		.besides(it => it.isValid = false)
		.remove();
}

Thread.self = function self()
{
	return currentSelf;
}

function Thread(options)
{
	options = options !== undefined ? options : {};

	this.threadID = null;
	this.threadPriority = options.priority !== undefined
		? options.priority : 0;
}

Thread.prototype.on_checkInput = function() {};
Thread.prototype.on_update = function() {};
Thread.prototype.on_render = function() {};

Object.defineProperty(Thread.prototype, 'running',
{
	enumerable: true, configurable: true,
	get: function get() { return Thread.isRunning(this.threadID); },
});

Thread.prototype.dispose = function dispose()
{
	this.stop();
};

Thread.prototype.join = function join()
{
	Thread.join(this.threadID);
};

Thread.prototype.start = function start()
{
	this.threadID = Thread.create({
		getInput: function() { this.on_checkInput(); }.bind(this),
		update:   function() { this.on_update(); return true }.bind(this),
		render:   function() { this.on_render(); }.bind(this),
	}, this.threadPriority);
};

Thread.prototype.stop = function stop()
{
	kill(this.threadID);
};

function _compare(a, b) {
	return a.priority != b.priority ?
		a.priority - b.priority :
		a.id - b.id;
};

function _makeThread(that, threadDesc)
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
	threads.sort(_compare);
	return newThread.id;
}

function _renderAll()
{
	let activeThreads = from.Array(threads.slice())
		.where(it => it.isValid)
		.where(it => it.renderer !== undefined)
    for (let thread of activeThreads)
		thread.renderer();
}

function _updateAll()
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
