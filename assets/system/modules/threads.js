/**
 *  miniRT threads CommonJS module
 *  (c) 2015-2016 Fat Cerberus
**/

'use strict';
module.exports =
{
	create:    create,
	isRunning: isRunning,
	join:      join,
	kill:      kill,
	self:      self,
};

const from = require('from');

var currentSelf = 0;
var haveMapEngine = typeof MapEngine === 'function';
var nextThreadID = 1;
var threads = [];
Dispatch.onUpdate(_updateAll);
Dispatch.onRender(_renderAll);

function create(entity, priority)
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

function isRunning(threadID)
{
	if (threadID == 0) return false;
	for (var i = 0; i < threads.length; ++i) {
		if (threads[i].id == threadID) {
			return true;
		}
	}
	return false;
}

function join(threadIDs)
{
	threadIDs = threadIDs instanceof Array ? threadIDs : [ threadIDs ];
	while (from.Array(threads)
		.where(function(t) { return threadIDs.indexOf(t.id) >= 0; })
		.count() > 0)
	{
		if (haveMapEngine && IsMapEngineRunning()) {
            UpdateMapEngine();
			RenderMap();
		}
		screen.flip();
	}
}

function kill(threadID)
{
	from.Array(threads)
		.where(function(t) { return t.id == threadID })
		.besides(function(t) { t.isValid = false; })
		.remove();
}

function self()
{
	return currentSelf;
}

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
	from.Array(threads.slice())
		.where(function(t) { return t.isValid; })
		.where(function(t) { return t.renderer !== undefined; })
		.each(function(thread)
	{
		thread.renderer();
	});
}

function _updateAll()
{
	var threadsEnding = [];
	from.Array(threads.slice())
		.where(function(t) { return t.isValid; })
		.where(function(t) { return !t.isBusy; })
		.each(function(thread)
	{
		var lastSelf = currentSelf;
		thread.isBusy = true;
		currentSelf = thread.id;
		var isRunning = thread.updater(thread.id);
		if (thread.inputHandler !== undefined && isRunning)
			thread.inputHandler();
		currentSelf = lastSelf;
		thread.isBusy = false;
		if (!isRunning)
			threadsEnding.push(thread.id);
	});
	from.Array(threadsEnding)
		.each(function(threadID)
	{
		kill(threadID);
	});
}
