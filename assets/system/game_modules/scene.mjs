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

import assert from 'assert';
import from from 'from';
import Prim from 'prim';
import Thread from 'thread';

let screenMask = Color.Transparent;

Dispatch.onRender(() =>
{
	if (screenMask.a > 0.0)
		Prim.fill(screen, screenMask);
}, 99);

export default
class Scene
{
	get [Symbol.toStringTag]() { return 'Scene'; }

	static defineOp(name, def)
	{
		if (name in this.prototype)
			throw new Error("scene op `" + name + "` is already defined");
		this.prototype[name] = function (...args) {
			this.enqueue({
				arguments: [ ...args ],
				start: def.start,
				getInput: def.getInput,
				update: def.update,
				render: def.render,
				finish: def.finish
			});
			return this;
		};
	}

	constructor()
	{
		this.timeline = new Timeline(this);
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
		let jump = { ifFalse: null };
		this.jumpsToFix.push(jump);
		let op = {
			arguments: [],
			start: function(scene) {
				if (predicate.call(scene))
					return;
				let timeline = Thread.self();
				timeline.goTo(jump.ifFalse);
			}
		};
		this.enqueue(op);
		this.openBlockTypes.push('branch');
		return this;
	}

	doWhile(predicate)
	{
		let jump = {
			loopStart: this.timeline.length,
			ifDone: null,
		};
		this.jumpsToFix.push(jump);
		let op = {
			arguments: [],
			start: function(scene) {
				if (predicate.call(scene))
					return;
				let timeline = Thread.self();
				timeline.goTo(jump.ifDone);
			}
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
		let jump;
		let op;
		switch (blockType) {
			case 'fork':
				op = {
					arguments: [ this.timeline ],
					start: function(scene, timeline) {
						// note: Thread.self() is the calling timeline
						let forkedFrom = Thread.self();
						forkedFrom.children.push(timeline);
						timeline.start();
					}
				};
				this.timeline = this.forkStack.pop();
				this.enqueue(op);
				break;
			case 'branch':
				jump = this.jumpsToFix.pop();
				jump.ifFalse = this.timeline.length;
				break;
			case 'loop':
				jump = this.jumpsToFix.pop();
				op = {
					arguments: [ jump ],
					start: function(scene, jump) {
						let timeline = Thread.self();
						timeline.goTo(jump.loopStart);
					}
				};
				this.enqueue(op);
				jump.ifDone = this.timeline.length;
				break;
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
		this.timeline = new Timeline(this);
		this.openBlockTypes.push('fork');
		return this;
	}

	resync()
	{
		let op = {
			arguments: [],
			start(scene) {
				let timeline = Thread.self();
				this.forks = timeline.children;
			},
			update(scene) {
				return from.Array(this.forks)
					.where(it => it.running)
					.count() > 0;
			}
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
		return Thread.join(this.timeline);
	}
}

class Timeline extends Thread
{
	constructor(scene)
	{
		super();

		this.children = [];
		this.ops = [];
		this.pc = 0;
		this.scene = scene;
	}

	get length()
	{
		return this.ops.length;
	}

	enqueue(op)
	{
		if (this.running)
			throw new Error("cannot enqueue scene op while running");
		this.ops.push(op);
	}

	goTo(pc)
	{
		this.pc = pc;
	}

	async on_update()
	{
		let op = this.ops[this.pc++];
		let thread = new OpThread(this.scene, op);
		await thread.start();
		await Thread.join(thread);
		if (this.pc >= this.ops.length)
			this.stop();
	}
}

class OpThread extends Thread
{
	constructor(scene, op)
	{
		super();

		this.scene = scene;
		this.op = op;
		this.context = {};
	}

	async start()
	{
		await this.op.start.call(this.context, this.scene,
			...this.op.arguments);
		if (this.op.update !== undefined) {
			super.start();
			if (this.op.getInput !== undefined)
				this.takeInput();
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

Scene.defineOp('call',
{
	start(scene, method, ...args) {
		method.apply(null, ...args);
	}
});

Scene.defineOp('fadeTo',
{
	start: function(scene, color, frames) {
		assert.ok(typeof frames === 'number');

		this.fader = new Scene()
			.tween(screenMask, frames, 'linear', color);
		this.fader.run();
	},
	update: function(scene) {
		return this.fader.running;
	}
});

Scene.defineOp('pause',
{
	start: function(scene, frames) {
		this.duration = frames;
		this.elapsed = 0;
	},
	update: function(scene) {
		++this.elapsed;
		return this.elapsed < this.duration;
	}
});

Scene.defineOp('playSound',
{
	start: function(scene, fileName) {
		this.sound = new Sound(fileName);
		this.sound.play(Mixer.Default);
		return true;
	},
	update: function(scene) {
		return this.sound.playing;
	}
});

Scene.defineOp('tween',
{
	start: function(scene, object, frames, easingType, endValues) {
		this.easers = {
			linear: function(t, b, c, d) {
				return c * t / d + b;
			},
			easeInQuad: function(t, b, c, d) {
				return c*(t/=d)*t + b;
			},
			easeOutQuad: function(t, b, c, d) {
				return -c *(t/=d)*(t-2) + b;
			},
			easeInOutQuad: function(t, b, c, d) {
				if ((t/=d/2) < 1) return c/2*t*t + b;
				return -c/2 * ((--t)*(t-2) - 1) + b;
			},
			easeInCubic: function(t, b, c, d) {
				return c*(t/=d)*t*t + b;
			},
			easeOutCubic: function(t, b, c, d) {
				return c*((t=t/d-1)*t*t + 1) + b;
			},
			easeInOutCubic: function(t, b, c, d) {
				if ((t/=d/2) < 1) return c/2*t*t*t + b;
				return c/2*((t-=2)*t*t + 2) + b;
			},
			easeInQuart: function(t, b, c, d) {
				return c*(t/=d)*t*t*t + b;
			},
			easeOutQuart: function(t, b, c, d) {
				return -c * ((t=t/d-1)*t*t*t - 1) + b;
			},
			easeInOutQuart: function(t, b, c, d) {
				if ((t/=d/2) < 1) return c/2*t*t*t*t + b;
				return -c/2 * ((t-=2)*t*t*t - 2) + b;
			},
			easeInQuint: function(t, b, c, d) {
				return c*(t/=d)*t*t*t*t + b;
			},
			easeOutQuint: function(t, b, c, d) {
				return c*((t=t/d-1)*t*t*t*t + 1) + b;
			},
			easeInOutQuint: function(t, b, c, d) {
				if ((t/=d/2) < 1) return c/2*t*t*t*t*t + b;
				return c/2*((t-=2)*t*t*t*t + 2) + b;
			},
			easeInSine: function(t, b, c, d) {
				return -c * Math.cos(t/d * (Math.PI/2)) + c + b;
			},
			easeOutSine: function(t, b, c, d) {
				return c * Math.sin(t/d * (Math.PI/2)) + b;
			},
			easeInOutSine: function(t, b, c, d) {
				return -c/2 * (Math.cos(Math.PI*t/d) - 1) + b;
			},
			easeInExpo: function(t, b, c, d) {
				return (t==0) ? b : c * Math.pow(2, 10 * (t/d - 1)) + b;
			},
			easeOutExpo: function(t, b, c, d) {
				return (t==d) ? b+c : c * (-Math.pow(2, -10 * t/d) + 1) + b;
			},
			easeInOutExpo: function(t, b, c, d) {
				if (t==0) return b;
				if (t==d) return b+c;
				if ((t/=d/2) < 1) return c/2 * Math.pow(2, 10 * (t - 1)) + b;
				return c/2 * (-Math.pow(2, -10 * --t) + 2) + b;
			},
			easeInCirc: function(t, b, c, d) {
				return -c * (Math.sqrt(1 - (t/=d)*t) - 1) + b;
			},
			easeOutCirc: function(t, b, c, d) {
				return c * Math.sqrt(1 - (t=t/d-1)*t) + b;
			},
			easeInOutCirc: function(t, b, c, d) {
				if ((t/=d/2) < 1) return -c/2 * (Math.sqrt(1 - t*t) - 1) + b;
				return c/2 * (Math.sqrt(1 - (t-=2)*t) + 1) + b;
			},
			easeInElastic: function(t, b, c, d) {
				var s=1.70158;var p=0;var a=c;
				if (t==0) return b;  if ((t/=d)==1) return b+c;  if (!p) p=d*.3;
				if (a < Math.abs(c)) { a=c; var s=p/4; }
				else var s = p/(2*Math.PI) * Math.asin (c/a);
				return -(a*Math.pow(2,10*(t-=1)) * Math.sin( (t*d-s)*(2*Math.PI)/p )) + b;
			},
			easeOutElastic: function(t, b, c, d) {
				var s=1.70158;var p=0;var a=c;
				if (t==0) return b;  if ((t/=d)==1) return b+c;  if (!p) p=d*.3;
				if (a < Math.abs(c)) { a=c; var s=p/4; }
				else var s = p/(2*Math.PI) * Math.asin (c/a);
				return a*Math.pow(2,-10*t) * Math.sin( (t*d-s)*(2*Math.PI)/p ) + c + b;
			},
			easeInOutElastic: function(t, b, c, d) {
				var s=1.70158;var p=0;var a=c;
				if (t==0) return b;  if ((t/=d/2)==2) return b+c;  if (!p) p=d*(.3*1.5);
				if (a < Math.abs(c)) { a=c; var s=p/4; }
				else var s = p/(2*Math.PI) * Math.asin (c/a);
				if (t < 1) return -.5*(a*Math.pow(2,10*(t-=1)) * Math.sin( (t*d-s)*(2*Math.PI)/p )) + b;
				return a*Math.pow(2,-10*(t-=1)) * Math.sin( (t*d-s)*(2*Math.PI)/p )*.5 + c + b;
			},
			easeInBack: function(t, b, c, d, s) {
				if (s == undefined) s = 1.70158;
				return c*(t/=d)*t*((s+1)*t - s) + b;
			},
			easeOutBack: function(t, b, c, d, s) {
				if (s == undefined) s = 1.70158;
				return c*((t=t/d-1)*t*((s+1)*t + s) + 1) + b;
			},
			easeInOutBack: function(t, b, c, d, s) {
				if (s == undefined) s = 1.70158;
				if ((t/=d/2) < 1) return c/2*(t*t*(((s*=(1.525))+1)*t - s)) + b;
				return c/2*((t-=2)*t*(((s*=(1.525))+1)*t + s) + 2) + b;
			},
			easeInBounce: function(t, b, c, d) {
				return c - this.easeOutBounce(d-t, 0, c, d) + b;
			},
			easeOutBounce: function(t, b, c, d) {
				if ((t/=d) < (1/2.75)) {
					return c*(7.5625*t*t) + b;
				} else if (t < (2/2.75)) {
					return c*(7.5625*(t-=(1.5/2.75))*t + .75) + b;
				} else if (t < (2.5/2.75)) {
					return c*(7.5625*(t-=(2.25/2.75))*t + .9375) + b;
				} else {
					return c*(7.5625*(t-=(2.625/2.75))*t + .984375) + b;
				}
			},
			easeInOutBounce: function(t, b, c, d) {
				if (t < d/2) return this.easeInBounce(t*2, 0, c, d) * .5 + b;
				return this.easeOutBounce(t*2-d, 0, c, d) * .5 + c*.5 + b;
			}
		};
		this.change = {};
		this.duration = frames;
		this.elapsed = 0.0;
		this.object = object;
		this.startValues = {};
		this.type = easingType in this.easers ? easingType : 'linear';
		var isChanged = false;
		for (var p in endValues) {
			this.change[p] = endValues[p] - object[p];
			this.startValues[p] = object[p];
			isChanged = isChanged || this.change[p] != 0;
		}
		if (!isChanged) {
			this.elapsed = this.duration;
		}
	},
	update: function(scene) {
		++this.elapsed;
		if (this.elapsed < this.duration) {
			for (var p in this.change) {
				this.object[p] = this.easers[this.type](this.elapsed, this.startValues[p], this.change[p], this.duration);
			}
			return true;
		} else {
			return false;
		}
	},
	finish: function(scene) {
		for (var p in this.change) {
			this.object[p] = this.startValues[p] + this.change[p];
		}
	}
});
