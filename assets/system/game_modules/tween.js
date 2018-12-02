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

let activeTweens = [];
let job = null;

export
const Easing =
{
	Linear: 0,
	Back: 1,
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
	constructor(target, easing = Easing.Sine)
	{
		const easeIn = easing === Easing.Back ? easeInBack
			: easing === Easing.Bounce ? easeInBounce
			: easing === Easing.Circular ? easeInCircular
			: easing === Easing.Cubic ? easeInCubic
			: easing === Easing.Elastic ? easeInElastic
			: easing === Easing.Exponential ? easeInExponential
			: easing === Easing.Quadratic ? easeInQuadratic
			: easing === Easing.Quartic ? easeInQuartic
			: easing === Easing.Quintic ? easeInQuintic
			: easing === Easing.Sine ? easeInSine
			: easeLinear;
		const easeOut = (t) => 1.0 - easeIn(1.0 - t);
		const easeInOut = (t) => t < 0.5 ? 0.5 * easeIn(t * 2.0) : 0.5 + 0.5 * easeOut(t * 2.0 - 1.0);
		this.inEaser = easeIn;
		this.inOutEaser = easeInOut;
		this.outEaser = easeOut;
		this.target = target;
	}

	async easeIn(newValues, numFrames)
	{
		await runTween(this.target, newValues, this.inEaser, numFrames);
	}

	async easeInOut(newValues, numFrames)
	{
		await runTween(this.target, newValues, this.inOutEaser, numFrames);
	}

	async easeOut(newValues, numFrames)
	{
		await runTween(this.target, newValues, this.outEaser, numFrames);
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
				tween.resolve();
			}
		}
		activeTweens.length = ptr;
	}, {
		inBackground: true,
		priority: Infinity,
	});
}

function easeInBack(t)
{
	return t * t * t - t * Math.sin(t * Math.PI);
}

function easeInBounce(t)
{
	t = 1.0 - t;
	const p = t < (1.0 / 2.75) ? 7.5625 * t * t
		: t < (2.0 / 2.75) ? 7.5625 * (t -= 1.5 / 2.75) * t + 0.75
		: t < (2.5 / 2.75) ? 7.5625 * (t -= 2.25 / 2.75) * t + 0.9375
		: 7.5625 * (t -= 2.625 / 2.75) * t + 0.984375;
	return 1.0 - p;
}

function easeInCircular(t)
{
	return 1.0 - Math.sqrt(1.0 - t * t);
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

function easeInSine(t)
{
	return Math.sin((t - 1.0) * Math.PI / 2) + 1.0;
}

function runTween(targetObject, newValues, easer, numFrames)
{
	const initialValues = {};
	const targetValues = {};
	for (const p of Object.keys(newValues)) {
		initialValues[p] = targetObject[p];
		targetValues[p] = newValues[p];
	}
	const promise = new Promise(resolve => {
		activeTweens.push({
			initialValues,
			targetValues,
			easer,
			resolve,
			targetObject,
			startTime: Sphere.now(),
			endTime: Sphere.now() + numFrames,
		});
	});
	appearifyUpdateJob();
	return promise;
}
