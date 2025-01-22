/**
 *  Oozaru: Sphere for the Web
 *  Copyright (c) 2016-2025, Where'd She Go? LLC
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

import { Deque } from './deque.js';
import Game from './game.js';

var defaultMixer = null;

export default
class Audialis
{
	static async initialize()
	{
		defaultMixer = new Mixer(44100, 16, 2);
	}
}

export
class Mixer
{
	#audioContext;
	#gainNode;
	#panNode;

	static get Default()
	{
		return defaultMixer;
	}

	constructor(sampleRate, bits, numChannels = 2)
	{
		this.#audioContext = new AudioContext({ sampleRate });
		this.#gainNode = this.#audioContext.createGain();
		this.#panNode = this.#audioContext.createStereoPanner();
		this.#gainNode.gain.value = 1.0;
		this.#gainNode
			.connect(this.#panNode)
			.connect(this.#audioContext.destination);
	}

	get pan()
	{
		return this.#panNode.pan.value;
	}

	get volume()
	{
		return this.#gainNode.gain.value;
	}

	set pan(value)
	{
		this.#panNode.pan.value = value;
	}

	set volume(value)
	{
		this.#gainNode.gain.value = value;
	}

	attachAudio(audioElement)
	{
		const audioNode = this.#audioContext.createMediaElementSource(audioElement);
		audioNode.connect(this.#gainNode);
		return audioNode;
	}

	attachScript(numChannels, callback)
	{
		const node = this.#audioContext.createScriptProcessor(0, 0, numChannels);
		node.onaudioprocess = (e) => callback(e.outputBuffer);
		node.connect(this.#gainNode);
		return node;
	}
}

export
class Sound
{
	#audioElement;
	#audioNode = null;
	#currentMixer = null;
	#fileName;

	static async fromFile(fileName)
	{
		const url = Game.urlOf(fileName);
		const audioElement = new Audio();
		await new Promise((resolve, reject) => {
			audioElement.onloadedmetadata = () => {
				resolve();
			}
			audioElement.onerror = () => {
				reject(Error(`Couldn't load audio file '${url}'.`));
			};
			audioElement.src = url;
		});
		const sound = new Sound(audioElement);
		sound.#fileName = Game.fullPath(fileName);
		return sound;
	}

	constructor(source)
	{
		if (source instanceof HTMLAudioElement) {
			this.#audioElement = source;
			this.#audioElement.loop = true;
		}
		else if (typeof source === 'string') {
			throw Error("'new Sound' from filename is not supported");
		}
		else {
			throw TypeError(`Invalid value '${source}' passed for 'Sound' source`);
		}
	}

	get fileName()
	{
		return this.#fileName;
	}

	get length()
	{
		return this.#audioElement.duration;
	}

	get playing()
	{
		return !this.#audioElement.paused;
	}

	get position()
	{
		return this.#audioElement.currentTime;
	}

	get repeat()
	{
		return this.#audioElement.loop;
	}

	get speed()
	{
		return this.#audioElement.playbackRate;
	}

	get volume()
	{
		return this.#audioElement.volume;
	}

	set position(value)
	{
		this.#audioElement.currentTime = value;
	}

	set repeat(value)
	{
		this.#audioElement.loop = value;
	}

	set speed(value)
	{
		this.#audioElement.playbackRate = value;
	}

	set volume(value)
	{
		this.#audioElement.volume = value;
	}

	pause()
	{
		this.#audioElement.pause();
	}

	play(mixer = Mixer.Default)
	{
		if (mixer !== this.#currentMixer) {
			this.#currentMixer = mixer;
			if (this.#audioNode !== null)
				this.#audioNode.disconnect();
			this.#audioNode = mixer.attachAudio(this.#audioElement);
		}
		this.#audioElement.play();
	}

	stop()
	{
		this.#audioElement.pause();
		this.#audioElement.currentTime = 0.0;
	}
}

export
class SoundStream
{
	#audioNode = null;
	#buffers = new Deque();
	#currentMixer = null;
	#inputPtr = 0.0;
	#numChannels;
	#paused = true;
	#sampleRate;
	#timeBuffered = 0.0;

	constructor(frequency = 22050, bits = 8, numChannels = 1)
	{
		if (bits != 32)
			throw RangeError("SoundStream bit depth must be 32-bit under Oozaru");
		this.#numChannels = numChannels;
		this.#sampleRate = frequency;
	}

	get length()
	{
		return this.#timeBuffered;
	}

	pause()
	{
		this.#paused = true;
	}

	play(mixer = Mixer.Default)
	{
		this.#paused = false;
		if (mixer !== this.#currentMixer) {
			if (this.#audioNode !== null)
				this.#audioNode.disconnect();
			this.#audioNode = mixer.attachScript(this.#numChannels, (buffer) => {
				const outputs = [];
				for (let i = 0; i < this.#numChannels; ++i)
					outputs[i] = buffer.getChannelData(i);
				if (this.#paused || this.#timeBuffered < buffer.duration) {
					// not enough data buffered or stream is paused, fill with silence
					for (let i = 0; i < this.#numChannels; ++i)
						outputs[i].fill(0.0);
					return;
				}
				this.#timeBuffered -= buffer.duration;
				if (this.#timeBuffered < 0.0)
					this.#timeBuffered = 0.0;
				const step = this.#sampleRate / buffer.sampleRate;
				let input = this.#buffers.first;
				let inputPtr = this.#inputPtr;
				for (let i = 0, len = outputs[0].length; i < len; ++i) {
					const t1 = Math.floor(inputPtr) * this.#numChannels;
					let t2 = t1 + this.#numChannels;
					const frac = inputPtr % 1.0;

					// FIXME: if `t2` is past the end of the buffer, the first sample from the
					//        NEXT buffer should be used, but actually doing that requires some
					//        reorganization, so just skip the interpolation for now.
					if (t2 >= input.length)
						t2 = t1;

					for (let j = 0; j < this.#numChannels; ++j) {
						const a = input[t1 + j];
						const b = input[t2 + j];
						outputs[j][i] = a + frac * (b - a);
					}
					inputPtr += step;
					if (inputPtr >= Math.floor(input.length / this.#numChannels)) {
						this.#buffers.shift();
						if (!this.#buffers.empty) {
							inputPtr -= Math.floor(input.length / this.#numChannels);
							input = this.#buffers.first;
						}
						else {
							// no more data, fill the rest with silence and return
							for (let j = 0; j < this.#numChannels; ++j)
								outputs[j].fill(0.0, i + 1);
							return;
						}
					}
				}
				this.#inputPtr = inputPtr;
			});
			this.#currentMixer = mixer;
		}
	}

	stop()
	{
		if (this.#audioNode !== null)
			this.#audioNode.disconnect();
		this.#buffers.clear();
		this.#inputPtr = 0.0;
		this.#currentMixer = null;
		this.#audioNode = null;
		this.#paused = true;
		this.#timeBuffered = 0.0;
	}

	write(data)
	{
		this.#buffers.push(data);
		this.#timeBuffered += data.length / (this.#sampleRate * this.#numChannels);
	}
}
