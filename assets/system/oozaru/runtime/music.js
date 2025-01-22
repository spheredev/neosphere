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

import Tween, { Easing } from './tween.js';

let adjuster = null;
let currentSound = null;
let haveOverride = false;
let mixer = null;
let oldSounds = [];
let topmostSound = null;

export default
class Music extends null
{
	static async adjustVolume(newVolume, fadeTime = 0)
	{
		appearifyMixer();
		newVolume = Math.min(Math.max(newVolume, 0.0), 1.0);
		if (fadeTime > 0)
			await adjuster.easeIn({ volume: newVolume }, fadeTime);
		else
			mixer.volume = newVolume;
	}

	static async override(fileName, fadeTime = 0)
	{
		await crossfade(fileName, fadeTime, true);
		haveOverride = true;
	}

	static async play(fileName, fadeTime = 0)
	{
		topmostSound = await crossfade(fileName, fadeTime, false);
	}

	static pop(fadeTime = 0)
	{
		if (oldSounds.length === 0)
			return;
		currentSound.tween.easeIn({ volume: 0.0 }, fadeTime);
		topmostSound = oldSounds.pop();
		currentSound = topmostSound;
		if (currentSound !== null) {
			currentSound.stream.volume = 0.0;
			currentSound.tween.easeIn({ volume: 1.0 }, fadeTime);
		}
	}

	static async push(fileName, fadeTime = 0)
	{
		let oldSound = topmostSound;
		await this.play(fileName, fadeTime);
		oldSounds.push(oldSound);
	}

	static reset(fadeTime = 0)
	{
		if (!haveOverride)
			return;
		haveOverride = false;

		currentSound.tween.easeIn({ volume: 0.0 }, fadeTime);
		currentSound = topmostSound;
		if (currentSound !== null) {
			currentSound.stream.volume = 0.0;
			currentSound.tween.easeIn({ volume: 1.0 }, fadeTime);
		}
	}
}

function appearifyMixer()
{
	// lazy mixer creation, works around autoplay policy in Oozaru
	if (mixer === null) {
		mixer = new Mixer(44100, 16, 2);
		adjuster = new Tween(mixer, Easing.Linear);
	}
}

async function crossfade(fileName, frames = 0, forceChange)
{
	appearifyMixer();
	let allowChange = !haveOverride || forceChange;
	if (currentSound !== null && allowChange)
		currentSound.tween.easeIn({ volume: 0.0 }, frames);
	if (fileName !== null) {
		let stream = await Sound.fromFile(fileName);
		stream.repeat = true;
		stream.volume = 0.0;
		stream.play(mixer);
		let newSound = { stream, tween: new Tween(stream, Easing.Linear) };
		if (allowChange) {
			newSound.tween.easeIn({ volume: 1.0 }, frames);
			currentSound = newSound;
		}
		return newSound;
	}
	else {
		return null;
	}
}
