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

let activeTweens = [];
let job = null;

export
const Easing =
{
	Linear: 0,
	BackUp: 1,
	Bounce: 2,
	Circular: 3,
	Cubic: 4,
	Elastic: 5,
	Exponential: 6,
	Quadratic: 7,
	Quartic: 8,
	Quintic: 9,
	Sine: 10,
};

export default
class Tween
{
	constructor(target, easing = Easing.Linear)
	{
		this.easing = easing;
		this.target = target;
	}

	async easeIn(newValues, numFrames)
	{
		const easer = this.easing === Easing.BackUp ? easeInBack
			: this.easing === Easing.Cubic ? easeInCubic
			: this.easing === Easing.Elastic ? easeInElastic
			: this.easing === Easing.Exponential ? easeInExponential
			: this.easing === Easing.Quadratic ? easeInQuadratic
			: this.easing === Easing.Quartic ? easeInQuartic
			: this.easing === Easing.Quintic ? easeInQuintic
			: easeLinear;
		await runTween(this.target, newValues, easer, numFrames);
	}

	async easeInOut(newValues, numFrames)
	{
		const easeIn = this.easing === Easing.BackUp ? easeInBack
			: this.easing === Easing.Cubic ? easeInCubic
			: this.easing === Easing.Elastic ? easeInElastic
			: this.easing === Easing.Exponential ? easeInExponential
			: this.easing === Easing.Quadratic ? easeInQuadratic
			: this.easing === Easing.Quartic ? easeInQuartic
			: this.easing === Easing.Quintic ? easeInQuintic
			: easeLinear;
		const easeOut = t => 1.0 - easeIn(1.0 - t);
		const easer = t => t < 0.5 ? 0.5 * easeIn(t * 2.0) : 0.5 + 0.5 * easeOut(t * 2.0 - 1.0);
		await runTween(this.target, newValues, easer, numFrames);
	}

	async easeOut(newValues, numFrames)
	{
		const easeIn = this.easing === Easing.BackUp ? easeInBack
			: this.easing === Easing.Cubic ? easeInCubic
			: this.easing === Easing.Elastic ? easeInElastic
			: this.easing === Easing.Exponential ? easeInExponential
			: this.easing === Easing.Quadratic ? easeInQuadratic
			: this.easing === Easing.Quartic ? easeInQuartic
			: this.easing === Easing.Quintic ? easeInQuintic
			: easeLinear;
		const easer = t => 1.0 - easeIn(1.0 - t);
		await runTween(this.target, newValues, easer, numFrames);
	}
}

function appearifyUpdateJob()
{
	if (job !== null)
		return;
	job = Dispatch.onUpdate(() => {
		let ptr = 0;
		for (let i = 0, len = activeTweens.length; i < len; ++i) {
			const tween = activeTweens[i];
			const len = tween.endTime - tween.startTime;
			const t = (Sphere.now() - tween.startTime) / len;
			if (t < 1.0) {
				activeTweens[ptr++] = tween;
				for (const p of Object.keys(tween.targetValues)) {
					const base = tween.initialValues[p];
					const delta = tween.targetValues[p] - base;
					tween.targetObject[p] = tween.easer(t) * delta + base;
				}
			}
			else {
				for (const p of Object.keys(tween.targetValues))
					tween.targetObject[p] = tween.targetValues[p];
				tween.pact.resolve();
			}
		}
		activeTweens.length = ptr;
	});
}

function easeInBack(t)
{
	return t * t * t - t * Math.sin(t * Math.PI);
}

function easeInCubic(t)
{
	return t * t * t;
}

function easeInElastic(t)
{
	return Math.sin(t * 7.5 * Math.PI) * 2.0 ** (10 * (t - 1.0));
}

function easeInExponential(t)
{
	return t === 0.0 ? t : 2 ** (10 * (t - 1.0));
}

function easeLinear(t)
{
	return t;
}

function easeInQuadratic(t)
{
	return t * t;
}

function easeInQuartic(t)
{
	return t * t * t * t;
}

function easeInQuintic(t)
{
	return t * t * t * t * t;
}

function runTween(targetObject, newValues, easer, numFrames)
{
	const initialValues = {};
	const targetValues = {};
	for (const p of Object.keys(newValues)) {
		initialValues[p] = targetObject[p];
		targetValues[p] = newValues[p];
	}
	const pact = new Pact();
	activeTweens.push({
		initialValues,
		targetValues,
		easer,
		pact,
		targetObject,
		startTime: Sphere.now(),
		endTime: Sphere.now() + numFrames,
	});
	appearifyUpdateJob();
	return pact;
}
