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

import Galileo from './galileo.js';

var frameCount = -1;
var frameRate = 60;
var jobSortNeeded = false;
var jobs = [];
var nextFrameTime = null;
var nextJobID = 1;
var rAFID = 0;

export
const JobType =
{
	// in order of execution
	Render: 0,
	Update: 1,
	Immediate: 2,
}

export default
class JobQueue
{
	static now()
	{
		return Math.max(frameCount, 0);
	}

	static start()
	{
		if (rAFID !== 0)  // already running?
			return;
		rAFID = requestAnimationFrame(animate);
	}

	static stop()
	{
		if (rAFID !== 0)
			cancelAnimationFrame(rAFID);
		frameCount = -1;
		jobs.length = 0;
		nextFrameTime = null;
		rAFID = 0;
	}
}

export
class Dispatch
{
	static cancelAll()
	{
		throw Error(`'Dispatch#cancelAll()' API is not implemented`);
	}

	static later(numFrames, callback)
	{
		const job = addJob(JobType.Update, callback, false, numFrames);
		return new JobToken(job);
	}

	static now(callback)
	{
		const job = addJob(JobType.Immediate, callback);
		return new JobToken(job);
	}

	static onRender(callback, options)
	{
		const job = addJob(JobType.Render, callback, true, 0, options);
		return new JobToken(job);
	}

	static onUpdate(callback, options)
	{
		const job = addJob(JobType.Update, callback, true, 0, options);
		return new JobToken(job);
	}
}

export
class JobToken
{
	#job;

	constructor(job)
	{
		this.#job = job;
	}

	cancel()
	{
		this.#job.cancelled = true;
	}

	pause()
	{
		this.#job.paused = true;
	}

	resume()
	{
		this.#job.paused = false;
	}
}

function addJob(type, callback, recurring = false, delay = 0, options)
{
	// for render jobs, invert priority so the highest-priority render is done last
	let priority = options?.priority ?? 0.0;
	if (type === JobType.Render)
		priority = -(priority);

	const job = {
		jobID: nextJobID++,
		type,
		callback,
		cancelled: false,
		priority,
		recurring,
		busy: false,
		paused: false,
		timer: delay,
	};
	jobs.push(job);
	jobSortNeeded = true;
	return job;
}

function animate(timestamp)
{
	rAFID = requestAnimationFrame(animate);

	const oldFrameCount = frameCount;
	nextFrameTime ??= timestamp;
	while (timestamp >= nextFrameTime) {
		++frameCount;
		nextFrameTime += 1000.0 / frameRate;

		// sort the Dispatch jobs for this frame
		if (jobSortNeeded) {
			// job queue sorting criteria, in order of key ranking:
			// 1. all recurring jobs first, followed by all one-offs
			// 2. renders, then updates, then immediates
			// 3. highest to lowest priority
			// 4. within the same priority bracket, maintain FIFO order
			jobs.sort((a, b) => {
				const recurDelta = +b.recurring - +a.recurring;
				const typeDelta = a.type - b.type;
				const priorityDelta = b.priority - a.priority;
				const fifoDelta = a.jobID - b.jobID;
				return recurDelta || typeDelta || priorityDelta || fifoDelta;
			});
			jobSortNeeded = false;
		}

		// this is a bit tricky.  Dispatch.now() is required to be processed in the same frame it's
		// issued, but we also want to avoid doing updates and renders out of turn.  to that end,
		// the loop below is split into two phases.  in phase one, we run through the sorted part of
		// the list.  in phase two, we process all jobs added since the frame started, but skip over
		// the update and render jobs, leaving them for the next frame.  conveniently for us,
		// Dispatch.now() jobs are not prioritized so they're guaranteed to be in the correct order
		// (FIFO) naturally!
		let ptr = 0;
		const initialLength = jobs.length;
		for (let i = 0; i < jobs.length; ++i) {
			const job = jobs[i];
			if ((i < initialLength || job.type === JobType.Immediate)
				&& (timestamp < nextFrameTime || job.type !== JobType.Render)
				&& !job.busy && !job.cancelled && (job.recurring || job.timer-- <= 0)
				&& !job.paused)
			{
				job.busy = true;
				Promise.resolve(job.callback()).then(() => {
					job.busy = false;
				})
				.catch((error) => {
					jobs.length = 0;
					throw error;
				});
			}
			if (job.cancelled || (!job.recurring && job.timer < 0))
				continue;  // delete it
			jobs[ptr++] = job;
		}
		jobs.length = ptr;
	}

	if (frameCount > oldFrameCount)
		Galileo.flip();
}
