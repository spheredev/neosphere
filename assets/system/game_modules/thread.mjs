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

import from from 'from';
import FocusTarget from 'focus-target';
import Pact from 'pact';

const
	CanDispatchOnExit = 'onExit' in Dispatch,
	CanPauseResumeJob = 'pause' in Dispatch.now(() => {});

export default
class Thread
{
	get [Symbol.toStringTag]() { return 'Thread'; }

	static join(...threads)
	{
		let threadPacts = from(threads).select(it => it._onThreadStop);
		return Promise.all(threadPacts);
	}

	constructor(options = {})
	{
		if (new.target === Thread)
			throw new Error(`'${new.target.name}' is abstract and cannot be instantiated`);

		options = Object.assign({}, {
			inBackground: false,
			priority:     0.0,
		}, options);

		this._bootstrapping = false;
		this._exitJob = null;
		this._focusTarget = new FocusTarget(options);
		this._inBackground = options.inBackground;
		this._onThreadStart = null;
		this._onThreadStop = Pact.resolve();
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
		if (!CanPauseResumeJob)
			throw new RangeError("Thread#pause requires newer Sphere version");
		if (!this.running)
			throw new Error("Thread is not running");
		this._updateJob.pause();
	}

	resume()
	{
		if (!CanPauseResumeJob)
			throw new RangeError("Thread#resume requires newer Sphere version");
		if (!this.running)
			throw new Error("Thread is not running");
		this._updateJob.resume();
	}

	async start()
	{
		if (this._started)
			return;

		this._bootstrapping = true;
		this._started = true;
		this._onThreadStart = new Pact();
		this._onThreadStop = new Pact();

		// Dispatch.onExit() ensures the shutdown handler is always called
		if (CanDispatchOnExit)
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
					this._onThreadStart.resolve();
					this._bootstrapping = false;
				}
				if (this.hasFocus)
					await this.on_inputCheck();
				await this.on_update();
			}, {
				inBackground: this._inBackground,
				priority:     this._priority,
			});

		// after thread terminates, remove it from the input queue
		this._onThreadStop.then(() => {
			this._focusTarget.dispose();
		});

		await this._onThreadStart;
	}

	async stop()
	{
		if (!this._started)
			return;

		this.yieldFocus();
		if (CanDispatchOnExit)
			this._exitJob.cancel();
		this._updateJob.cancel();
		this._renderJob.cancel();
		this._started = false;
		await this.on_shutDown();
		this._onThreadStop.resolve();
	}

	takeFocus()
	{
		if (!this.running)
			throw new Error("Thread is not running");
		if (this.on_inputCheck === Thread.on_inputCheck)
			throw new TypeError("Thread is not enabled for user input");

		this._focusTarget.takeFocus();
	}

	yieldFocus()
	{
		if (!this.running)
			throw new Error("Thread is not running");
		if (this.on_inputCheck === Thread.on_inputCheck)
			throw new TypeError("Thread is not enabled for user input");

		this._focusTarget.yield();
	}
}
