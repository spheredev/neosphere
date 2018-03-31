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

const now = SSj.now;

export default new
class Kami
{
	constructor()
	{
		this.initialized = false;

		this.enabled = false;
	}

	initialize(options = {})
	{
		options = Object.assign({
			includeEventLoop:  true,
			sortResultsByCost: true,
		}, Sphere.Game.kamiOptions, options);

		this.enabled = SSj.now() > 0;
		this.options = options;
		this.placeholder = new Record();
		this.records = [];
		if (this.enabled)
			this.exitJob = Dispatch.onExit(() => this.finish());

		this.initialized = true;
	}

	attach(target, methodName, description = `${target.constructor.name}#${methodName}`)
	{
		if (!this.initialized)
			this.initialize();

		if (!this.enabled || typeof target[methodName] !== 'function')
			return;

		let originalFunction = target[methodName];
		let record = new Record(description, true, methodName, originalFunction, target);
		this.records.push(record);

		target[methodName] = function (...args) {
			let startTime = now();
			let result = originalFunction.apply(this, args);
			record.totalTime += now() - startTime;
			++record.numSamples;
			return result;
		};
	}

	attachClass(constructor, name = constructor.name)
	{
		// attach all the static methods first
		for (const key of Object.getOwnPropertyNames(constructor))
			this.attach(constructor, key, `${name}.${key}`);

		// attach instance methods
		for (const key of Object.getOwnPropertyNames(constructor.prototype)) {
			if (key === 'constructor')
				continue;  // bad things will happen if we attach this
			this.attach(constructor.prototype, key, `${name}#${key}`);
		}
	}

	begin(description = "Unknown")
	{
		if (!this.initialized)
			this.initialize();

		if (this.enabled) {
			let record = findRecord(this.records, description);
			if(record === undefined) {
				record = new Record(description, false);
				this.records.push(record);
			}
			if (record.startTime > 0)
				throw RangeError("Reentrant profiling not supported");
			record.startTime = now();
			return record;
		}
		else {
			return this.placeholder;
		}
	}

	end(record)
	{
		let end = now();
		if (!(record instanceof Record))
			throw TypeError("Expected sample record from Kami.begin()");
		record.totalTime += end - record.startTime;
		record.startTime = 0;
		++record.numSamples;
	}

	finish()
	{
		if (!this.enabled)
			return;

		let runningTime = SSj.now();

		// cancel the onExit() so we don't end up printing the table twice
		this.exitJob.cancel();

		if (this.options.includeEventLoop) {
			let record = new Record("[in Sphere event loop]", false);
			record.numSamples = Sphere.now();
			record.totalTime = SSj.lostTime();
			this.records.push(record);
		}
		else {
			runningTime -= SSj.lostTime();
		}

		let totalTime = 0;
		let totalAverage = 0;
		let totalSamples = 0;
		for (const record of this.records) {
			if (record.numSamples > 0) {
				record.averageTime = record.totalTime / record.numSamples;
				totalSamples += record.numSamples;
				totalTime += record.totalTime;
				totalAverage += record.averageTime;
			}
		}
		if (this.options.sortResultsByCost) {
			this.records.sort((a, b) => b.totalTime - a.totalTime);
		}
		else {
			this.records.sort((a, b) => {
				return a.description > b.description ? 1
					: a.description < b.description ? -1
					: 0;
			});
		}

		// compile and output the Profiling Results table
		let consoleOutput = [
			[ "Event" ],
			[ "Count" ],
			[ "Time (\u{3bc}s)" ],
			[ "% Run" ],
			[ "Avg (\u{3bc}s)" ],
			[ "% Avg" ],
		];
		if (!this.options.includeEventLoop)
			consoleOutput[3][0] += "*";
		for (const record of this.records) {
			if (record.numSamples > 0) {
				consoleOutput[0].push(record.description);
				consoleOutput[1].push(toCountString(record.numSamples));
				consoleOutput[2].push(toTimeString(record.totalTime));
				consoleOutput[3].push(toPercentString(record.totalTime / runningTime));
				consoleOutput[4].push(toTimeString(record.averageTime));
				consoleOutput[5].push(toPercentString(record.averageTime / totalAverage));
			}
			if (record.attached) {
				// reinstall the original method now that we're done profiling
				record.target[record.methodName] = record.originalFunction;
			}
		}
		consoleOutput[0].push("Total");
		consoleOutput[1].push(toCountString(totalSamples));
		consoleOutput[2].push(toTimeString(totalTime));
		consoleOutput[3].push(toPercentString(totalTime / runningTime));
		consoleOutput[4].push(toTimeString(totalAverage));
		consoleOutput[5].push(toPercentString(1.0));

		let compiledText = `Profiling Results for '${Sphere.Game.name}' (Kami)`
			+ `\n${makeTable(consoleOutput)}\n`;
		if (!this.options.includeEventLoop)
			compiledText += "\n(*) Excludes time spent in the Sphere event loop.\n";
		SSj.log(compiledText);

		this.records.splice(0, this.records.length);
	}
};

function Record(description, attached, methodName, originalFunction, target)
{
	this.description = description;
	this.attached = attached;
	this.numSamples = 0;
	this.methodName = methodName;
	this.originalFunction = originalFunction;
	this.target = target;
	this.startTime = attached ? 1 : 0;
	this.totalTime = 0;
	this.averageTime = 0;
}

function findRecord(records, description)
{
	let i = 0;
	let length = records.length;
	for (; i < length && records[i].description !== description; ++i);
	return i < length ? records[i] : undefined;
}

function makeTable(table)
{
	let totalLength = 1;
	const rows = table[0].length;
	const columns = table.length;
	let output = "";

	for (let i = 0; i < table.length; ++i) {
		let width = 0;
		const column = table[i];
		for (const value of column)
			width = Math.max(width, value.length);
		for (let j = 0; j < rows; ++j) {
			if (i > 0)
				column[j] = `| ${column[j].padStart(width)} `;
			else
				column[j] = `${column[j].padEnd(width)} `;
		}
		totalLength += width + 3;
	}

	const startLine = `\n${"-".repeat(totalLength - 4)}`;

	for (let i = 0; i < rows; ++i) {
		if (i === rows - 1)
			output += startLine;

		output += "\n";
		for (let j = 0; j < columns; ++j)
			output += table[j][i];

		if (i === 0)
			output += startLine;
	}
	return output;
}

function toCountString(value)
{
	return value.toLocaleString();
}

function toPercentString(value)
{
	return `${(value * 100).toLocaleString(undefined, {
		maximumFractionDigits: 1,
		minimumFractionDigits: 1,
	})} %`;
}

function toTimeString(value)
{
	return Math.round(value / 1000).toLocaleString();
}