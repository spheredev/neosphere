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

const link = require('link');

var currentSelf = 0;
var haveMapEngine = typeof MapEngine === 'function';
var nextThreadID = 1;
var threads = [];
system.recur(Defer.Update, _updateAll);
system.recur(Defer.Render, _renderAll);

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
	while (link(threads)
		.filterBy('id', threadIDs)
		.length() > 0)
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
	link(threads)
		.where(function(thread) { return thread.id == threadID })
		.execute(function(thread) { thread.isValid = false; })
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
	return newThread.id;
}

function _renderAll()
{
	link(link(threads).sort(_compare))
		.where(function(thread) { return thread.isValid; })
		.where(function(thread) { return thread.renderer !== undefined; })
		.each(function(thread)
	{
		thread.renderer();
	});
}

function _updateAll()
{
	var threadsEnding = [];
	link(link(threads).toArray())
		.where(function(thread) { return thread.isValid; })
		.where(function(thread) { return !thread.isBusy; })
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
	link(threadsEnding)
		.each(function(threadID)
	{
		kill(threadID);
	});
}
