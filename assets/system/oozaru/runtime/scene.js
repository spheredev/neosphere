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

import Task from './task.js';

let defaultPriority = 0.0;

export default
class Scene
{
	get [Symbol.toStringTag]() { return 'Scene'; }

	static get defaultPriority()
	{
		return defaultPriority;
	}

	static set defaultPriority(value)
	{
		defaultPriority = value;
	}

	static defineOp(name, def)
	{
		if (name in this.prototype)
			throw new Error(`Scene op '${name}' is already defined`);
		this.prototype[name] = function (...args) {
			this.enqueue({
				arguments: [ ...args ],
				start: def.start,
				getInput: def.getInput,
				update: def.update,
				render: def.render,
				finish: def.finish,
			});
			return this;
		};
	}

	constructor(options)
	{
		options = Object.assign({}, {
			inBackground: false,
			priority: defaultPriority,
		}, options);

		this.threadOptions = options;
		this.timeline = new Timeline(this, this.threadOptions);
		this.forkStack = [];
		this.jumpsToFix = [];
		this.openBlockTypes = [];
	}

	get running()
	{
		return this.timeline.running;
	}

	doIf(predicate)
	{
		let timeline = this.timeline;
		let jump = { ifFalse: null };
		this.jumpsToFix.push(jump);
		let op = {
			arguments: [],
			start(scene) {
				if (predicate.call(scene))
					return;
				timeline.goTo(jump.ifFalse);
			},
		};
		this.enqueue(op);
		this.openBlockTypes.push('branch');
		return this;
	}

	doWhile(predicate)
	{
		let timeline = this.timeline;
		let jump = {
			loopStart: this.timeline.length,
			ifDone: null,
		};
		this.jumpsToFix.push(jump);
		let op = {
			arguments: [],
			start(scene) {
				if (predicate.call(scene))
					return;
				timeline.goTo(jump.ifDone);
			},
		};
		this.enqueue(op);
		this.openBlockTypes.push('loop');
		return this;
	}

	end()
	{
		if (this.openBlockTypes.length === 0)
			throw new Error("extraneous end() in scene definition");
		let blockType = this.openBlockTypes.pop();
		switch (blockType) {
			case 'fork': {
				let forkedFrom = this.forkStack.pop();
				let op = {
					arguments: [ this.timeline ],
					start(scene, timeline) {
						forkedFrom.children.push(timeline);
						timeline.goTo(0);
						timeline.start();
					},
				};
				this.timeline = forkedFrom;
				this.enqueue(op);
				break;
			}
			case 'branch': {
				let jump = this.jumpsToFix.pop();
				jump.ifFalse = this.timeline.length;
				break;
			}
			case 'loop': {
				let jump = this.jumpsToFix.pop();
				let op = {
					arguments: [ jump, this.timeline ],
					start(scene, jump, timeline) {
						timeline.goTo(jump.loopStart);
					},
				};
				this.enqueue(op);
				jump.ifDone = this.timeline.length;
				break;
			}
		}
		return this;
	}

	enqueue(op)
	{
		this.timeline.enqueue(op);
		return this;
	}

	fork()
	{
		this.forkStack.push(this.timeline);
		this.timeline = new Timeline(this, this.threadOptions);
		this.openBlockTypes.push('fork');
		return this;
	}

	resync()
	{
		let op = {
			arguments: [ this.timeline ],
			start(scene, timeline) {
				this.forks = timeline.children;
			},
			update(scene) {
				return this.forks.some(it => it.running);
			},
		};
		this.enqueue(op);
		return this;
	}

	async run()
	{
		if (this.openBlockTypes.length > 0)
			throw new Error("missing end() in scene definition");
		this.timeline.goTo(0);
		this.timeline.start();
		return Task.join(this.timeline);
	}

	stop()
	{
		this.timeline.stop();
	}
}

class Timeline extends Task
{
	constructor(scene, threadOptions)
	{
		super(threadOptions);

		this.children = [];
		this.opThread = null;
		this.ops = [];
		this.pc = 0;
		this.scene = scene;
		this.threadOptions = threadOptions;
	}

	get length()
	{
		return this.ops.length;
	}

	enqueue(op)
	{
		if (this.running)
			throw new Error("Cannot enqueue while scene is running");
		this.ops.push(op);
	}

	goTo(pc)
	{
		this.pc = pc;
	}

	stop()
	{
		if (this.opThread !== null)
			this.opThread.stop();
		for (const child of this.children)
			child.stop();
		super.stop();
	}

	async on_update()
	{
		let op = this.ops[this.pc++];
		let thread = new OpThread(this.scene, op, this.threadOptions);
		await thread.start();
		await Task.join(thread);
		if (this.pc >= this.ops.length) {
			await Task.join(...this.children);
			this.stop();
		}
	}
}

class OpThread extends Task
{
	constructor(scene, op, threadOptions)
	{
		super(threadOptions);

		this.scene = scene;
		this.op = op;
		this.context = {};
	}

	async start()
	{
		await this.op.start.call(this.context, this.scene, ...this.op.arguments);
		if (this.op.update !== undefined) {
			super.start();
			if (this.op.getInput !== undefined)
				this.takeFocus();
		}
		else {
			if (this.op.finish !== undefined)
				await this.op.finish.call(this.context, this.scene);
		}
	}

	async stop()
	{
		super.stop();
		if (this.op.finish !== undefined)
			await this.op.finish.call(this.context, this.scene);
	}

	async on_inputCheck()
	{
		await this.op.getInput.call(this.context, this.scene);
	}

	on_render()
	{
		if (this.op.render !== undefined)
			this.op.render.call(this.context, this.scene);
	}

	async on_update()
	{
		if (!await this.op.update.call(this.context, this.scene))
			await this.stop();
	}
}
