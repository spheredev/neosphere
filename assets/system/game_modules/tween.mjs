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

import Pact from 'pact';

export default
class Tween
{
	constructor(target, options = {})
	{
		options = Object.assign({}, {
			priority: Infinity,
		}, options);

		this.endTime = this.startTime;
		this.job = null;
		this.next = {};
		this.priority = options.priority;
		this.start = {};
		this.startTime = Sphere.now();
		this.target = target;
	}

	get tweening()
	{
		return this.job !== null;
	}

	stop(finishTween = true)
	{
		if (this.job !== null) {
			this.pact.resolve();
			this.job.cancel();
			this.job = null;
			if (finishTween) {
				for (const p of Object.keys(this.next))
					this.target[p] = this.next[p];
			}
		}
	}

	async tweenTo(newValues, numFrames)
	{
		this.startTime = Sphere.now();
		this.endTime = this.startTime + numFrames;
		if (this.job !== null) {
			this.stop(false);
			for (const p of Object.keys(this.next)) {
				if (!(p in newValues))
					this.target[p] = this.next[p];
			}
		}
		if (this.endTime > this.startTime) {
			this.start = {};
			this.next = {};
			for (const p of Object.keys(newValues)) {
				this.start[p] = this.target[p];
				this.next[p] = newValues[p];
			}
			this.pact = new Pact();
			this.job = Dispatch.onUpdate(() => {
				const t = Sphere.now() - this.startTime;
				const d = this.endTime - this.startTime;
				for (const p of Object.keys(this.next)) {
					const delta = this.next[p] - this.start[p];
					this.target[p] = lerp(this.start[p], delta, t, d);
				}
				if (t === d) {
					this.pact.resolve();
					this.job.cancel();
					this.job = null;
				}
			}, {
				inBackground: true,
				priority: this.priority,
			});
			await this.pact;
		}
		else {
			for (const p of Object.keys(newValues))
				this.target[p] = newValues[p];
		}
	}
}

function lerp(start, delta, t, d)
{
	return start + delta * (t / d);
}
