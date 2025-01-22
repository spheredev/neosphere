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

var jobs = [];

export default
class Fido
{
	static get numJobs()
	{
		return jobs.length;
	}

	static get progress()
	{
		let bytesTotal = 0;
		let bytesDone = 0;
		for (const job of jobs) {
			if (job.totalSize === null)
				continue;
			bytesTotal += job.totalSize;
			bytesDone += job.bytesDone;
		}
		return bytesTotal > 0 ? bytesDone / bytesTotal : 1.0;
	}

	static async fetch(url)
	{
		const response = await fetch(url);
		if (!response.ok || response.body === null)
			throw Error(`Couldn't fetch the file '${url}'. (HTTP ${response.status})`);
		const job = {
			url,
			bytesDone: 0,
			totalSize: null,
			finished: false,
		};
		jobs.push(job);
		const reader = response.body.getReader();
		const length = response.headers.get('Content-Length');
		if (length !== null)
			job.totalSize = parseInt(length, 10);
		const chunks = [];
		while (!job.finished) {
			const result = await reader.read();
			if (!result.done) {
				chunks.push(result.value);
				job.bytesDone += result.value.length;
			}
			job.finished = result.done;
		}
		let allDone = true;
		for (const job of jobs)
			allDone = allDone && job.finished;
		if (allDone)
			jobs.length = 0;
		return new Blob(chunks);
	}

	static async fetchData(url)
	{
		const blob = await this.fetch(url);
		return blob.arrayBuffer();
	}

	static async fetchImage(url)
	{
		const blob = await this.fetch(url);
		return new Promise((resolve, reject) => {
			const image = new Image();
			image.onload = () => {
				resolve(image);
				URL.revokeObjectURL(image.src);
			};
			image.onerror = () => {
				reject(Error(`Couldn't load image '${url}'`));
				URL.revokeObjectURL(image.src);
			}
			image.src = URL.createObjectURL(blob);
		});
	}

	static async fetchJSON(url)
	{
		const text = await this.fetchText(url);
		return JSON.parse(text);
	}

	static async fetchText(url)
	{
		const blob = await this.fetch(url);
		return blob.text();
	}
}
