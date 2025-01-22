/**
 *  Sphere: the JavaScript game platform
 *  Copyright (c) 2015-2022, Fat Cerberus
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
 *  * Neither the name of Spherical nor the names of its contributors may be
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

import FocusTarget from './focus-target.js';
import Pact from './pact.js';

const canDispatchOnExit = 'onExit' in Dispatch;
const canPauseJobs = 'pause' in Dispatch.now(() => {});

export default
class Task
{
	get [Symbol.toStringTag]() { return 'Task'; }

	static async join(...tasks)
	{
		const promises = tasks.map(it => it._onTaskStop);
		return Promise.all(promises);
	}

	constructor(options = {})
	{
		if (new.target === Task)
			throw new Error(`'${new.target.name}' is abstract and cannot be instantiated`);

		options = Object.assign({}, {
			inBackground: false,
			priority:     0.0,
		}, options);

		this._bootstrapping = false;
		this._exitJob = null;
		this._focusTarget = new FocusTarget(options);
		this._inBackground = options.inBackground;
		this._onTaskStart = null;
		this._onTaskStop = Pact.resolve();
		this._priority = options.priority;
		this._renderJob = null;
		this._started = false;
		this._updateJob = null;
	}

	get hasFocus()
	{
		return this._focusTarget.hasFocus;
	}

	get priority()
	{
		return this._priority;
	}

	get running()
	{
		return this._started;
	}

	on_startUp() {}
	on_shutDown() {}
	on_inputCheck() {}
	on_render() {}
	on_update() {}

	pause()
	{
		if (!canPauseJobs)
			throw new RangeError("Task#pause requires a newer Sphere version");
		if (!this.running)
			throw new Error("Task is not running");
		this._updateJob.pause();
	}

	resume()
	{
		if (!canPauseJobs)
			throw new RangeError("Task#resume requires a newer Sphere version");
		if (!this.running)
			throw new Error("Task is not running");
		this._updateJob.resume();
		this._renderJob.resume();
	}

	async start()
	{
		if (this._started)
			return;

		this._bootstrapping = true;
		this._started = true;
		this._onTaskStart = new Pact();
		this._onTaskStop = new Pact();

		// Dispatch.onExit() ensures the shutdown handler is always called
		if (canDispatchOnExit)
			this._exitJob = Dispatch.onExit(() => this.on_shutDown());

		// set up the update and render callbacks
		this._renderJob = Dispatch.onRender(
			() => {
				if (this._bootstrapping)
					return;
				this.on_render();
			}, {
				inBackground: this._inBackground,
				priority:     this._priority,
			});
		this._updateJob = Dispatch.onUpdate(
			async () => {
				if (this._bootstrapping) {
					await this.on_startUp();
					this._onTaskStart.resolve();
					this._bootstrapping = false;
				}
				if (this.hasFocus)
					await this.on_inputCheck();
				await this.on_update();
			}, {
				inBackground: this._inBackground,
				priority:     this._priority,
			});

		// after task terminates, remove it from the input queue
		this._onTaskStop.then(() => {
			this._focusTarget.dispose();
		});

		return this._onTaskStart;
	}

	async stop()
	{
		if (!this._started)
			return;

		this.yieldFocus();
		if (canDispatchOnExit)
			this._exitJob.cancel();
		this._updateJob.cancel();
		this._renderJob.cancel();
		this._started = false;
		await this.on_shutDown();
		this._onTaskStop.resolve();
	}

	suspend()
	{
		if (!canPauseJobs)
			throw new RangeError("Task#suspend requires a newer Sphere version");
		if (!this.running)
			throw new Error("Task is not running");
		this._updateJob.pause();
		this._renderJob.pause();
	}

	takeFocus()
	{
		if (!this.running)
			throw new Error("Task is not running");
		if (this.on_inputCheck === Task.on_inputCheck)
			throw new TypeError("Task is not enabled for user input");

		this._focusTarget.takeFocus();
	}

	yieldFocus()
	{
		if (!this.running)
			throw new Error("Task is not running");
		if (this.on_inputCheck === Task.on_inputCheck)
			throw new TypeError("Task is not enabled for user input");

		this._focusTarget.yield();
	}
}
