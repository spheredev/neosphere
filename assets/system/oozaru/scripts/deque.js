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

// this is based on an algorithm for dynamically-expanding ring buffers which
// is described here:
// https://blog.labix.org/2010/12/23/efficient-algorithm-for-expanding-circular-buffers

export
class Deque
{
	#backPtr = 0;
	#entries;
	#frontPtr = 0;
	#overflowPtr;
	#stride;
	#vips = [];

	constructor(reserveSize = 8)
	{
		this.#stride = reserveSize + 1;
		this.#entries = new Array(this.#stride);
		this.#overflowPtr = this.#stride;
	}

	*[Symbol.iterator]()
	{
		while (!this.empty)
			yield this.shift();
	}

	get empty()
	{
		return this.#backPtr === this.#frontPtr
			&& this.#overflowPtr === this.#stride
			&& this.#vips.length === 0;
	}

	get first()
	{
		return this.#vips.length > 0 ? this.#vips[this.#vips.length - 1]
			: this.#backPtr !== this.#frontPtr ? this.#entries[this.#frontPtr]
			: this.#entries[this.#stride];
	}

	get last()
	{
		const ptr = this.#backPtr > 0 ? this.#backPtr - 1
			: this.#stride - 1;
		return this.#overflowPtr > this.#stride ? this.#entries[this.#overflowPtr - 1]
			: this.#frontPtr !== this.#backPtr ? this.#entries[ptr]
			: this.#vips[0];
	}

	clear()
	{
		this.#entries.length = 0;
		this.#stride = 1;
		this.#overflowPtr = 1;
		this.#backPtr = 0;
		this.#frontPtr = 0;
	}

	pop()
	{
		if (this.#overflowPtr > this.#stride) {
			// take from the overflow area first
			return this.#entries[--this.#overflowPtr];
		}
		else if (this.#frontPtr !== this.#backPtr) {
			if (--this.#backPtr < 0)
				this.#backPtr = this.#stride - 1;
			return this.#entries[this.#backPtr];
		}
		else {
			// note: uses Array#shift so not O(1).  i'll fix it eventually but
			//       ultimately, I don't expect this case to be common.
			return this.#vips.shift();
		}
	}

	push(value)
	{
		const ringFull = (this.#backPtr + 1) % this.#stride === this.#frontPtr;
		if (ringFull || this.#overflowPtr > this.#stride) {
			// if there's already an overflow area established, we need to keep
			// using it to maintain proper FIFO order.
			this.#entries[this.#overflowPtr++] = value;
		}
		else {
			this.#entries[this.#backPtr++] = value;
			if (this.#backPtr >= this.#stride)
				this.#backPtr = 0;
		}
	}

	shift()
	{
		if (this.#vips.length > 0) {
			return this.#vips.pop();
		}
		else {
			const value = this.#entries[this.#frontPtr++];
			if (this.#frontPtr >= this.#stride)
				this.#frontPtr = 0;
			if (this.#frontPtr === this.#backPtr) {
				// absorb the overflow area back into the ring
				this.#frontPtr = this.#stride % this.#overflowPtr;
				this.#backPtr = 0;
				this.#stride = this.#overflowPtr;
			}
			return value;
		}
	}

	unshift(value)
	{
		const ringFull = (this.#backPtr + 1) % this.#stride === this.#frontPtr;
		if (!ringFull) {
			if (--this.#frontPtr < 0)
				this.#frontPtr = this.#stride - 1;
			this.#entries[this.#frontPtr] = value;
		}
		else {
			this.#vips.push(value);
		}
	}
}
