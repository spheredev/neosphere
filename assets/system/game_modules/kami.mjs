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
		this.enabled = now() > 0;
		this.records = [];
		this.useTableBorders = false;
		this.placeholder = new Record();
		if (this.enabled)
			this.exitJob = Dispatch.onExit(() => this.finish());
	}

	finish()
	{
		if (!this.enabled)
			return;

		this.exitJob.cancel();
		let totalTime = 0;
		let totalAverage = 0;
		let calls = 0;
		for (const record of this.records) {
			if (record.count > 0) {
				record.averageTime = record.totalTime / record.count;
				calls += record.count;
				totalTime += record.totalTime;
				totalAverage += record.averageTime;
			}
		}
		this.records.sort(function(a, b) { return b.averageTime - a.averageTime; });

		let consoleOutput = [
			[ "Event" ],
			[ "Samples" ],
			[ "Time [µs]" ],
			[ "% Total" ],
			[ "Avg [µs]" ],
			[ "% Avg" ],
		];
		for (const record of this.records) {
			if (record.count > 0) {
				consoleOutput[0].push(record.description);
				consoleOutput[1].push(toCountString(record.count));
				consoleOutput[2].push(toTimeString(record.totalTime));
				consoleOutput[3].push(toPercentString(record.totalTime / totalTime));
				consoleOutput[4].push(toTimeString(record.averageTime));
				consoleOutput[5].push(toPercentString(record.averageTime / totalAverage));
			}
			if (record.attached)
				record.target[record.methodName] = record.originalFunction;
		}
		consoleOutput[0].push("Total");
		consoleOutput[1].push(toCountString(calls));
		consoleOutput[2].push(toTimeString(totalTime));
		consoleOutput[3].push(toPercentString(1.0));
		consoleOutput[4].push(toTimeString(totalAverage));
		consoleOutput[5].push(toPercentString(1.0));

		SSj.log(`Profiling Results for '${Sphere.Game.name}' [Kami]`
			+ `\n${makeTable(consoleOutput, this.useTableBorders)}\n`);
		this.records.splice(0, this.records.length);
	}

	begin(description = "Unknown")
	{
		if (!this.enabled)
			return this.placeholder;
		
		let record = findRecord(this.records, description);
		if(record === undefined) {
			record = new Record(description, false, undefined, undefined, undefined);
			this.records.push(record);
		}
		if (record.startTime > 0)
			throw new Error("Reentrant profiling not supported");
		record.startTime = now();
		return record;
	}

	end(record)
	{
		let end = now();
		record.totalTime += end - record.startTime;
		record.startTime = 0;
		++record.count;
	}

	profile(target, methodName, description = `${target.constructor.name}#${methodName}`)
	{
		if (!this.enabled)
			return;

		let originalFunction = target[methodName];
		let record = new Record(description, true, methodName, originalFunction, target);
		this.records.push(record);

		target[methodName] = function (...args) {
			let startTime = now();
			let result = originalFunction.apply(this, args);
			record.totalTime += now() - startTime;
			++record.count;
			return result;
		};
	}
};

function Record(description, attached, methodName, originalFunction, target)
{
	this.description = description;
	this.attached = attached;
	this.count = 0;
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

function makeTable(table, useFullBorders)
{
	let totalLength = 1;
	let rows = table[0].length;
	let columns = table.length;
	let output = "";

	for (let i = 0; i < table.length; ++i) {
		let width = 0;
		const column = table[i];
		for (const value of column) {
			width = Math.max(width, value.length);
		}
		for (let j = 0; j < rows; ++j) {
			if (i > 0)
				column[j] = ` ${column[j].padStart(width)} `;
			else
				column[j] = `${column[j].padEnd(width)} `;
			if (useFullBorders)
				column[j] += "|";
		}
		totalLength += useFullBorders ? width + 3 : width + 2;
	}

	let start = "\n";
	let line = "";
	let startLine = "";
	if (useFullBorders) {
		start = "\n|";
		line = `\n${"-".repeat(totalLength - 1)}`;
	}
	else {
		startLine = `\n${"-".repeat(totalLength - 1)}`;
	}
	
	for (let i = 0; i < rows; ++i) {
		if (i === rows - 1)
			output += startLine;

		output += line + start;
		for (let j = 0; j < columns; ++j)
			output += table[j][i];

		if (i === 0)
			output += startLine;
	}
	output += line;
	return output;
}

function toCountString(value)
{
	return value.toLocaleString();
}

function toPercentString(value)
{
	return value.toLocaleString(undefined, {
		style: 'percent',
		maximumFractionDigits: 2,
		minimumFractionDigits: 2,
	});
}

function toTimeString(value)
{
	return `${Math.round(value / 1000).toLocaleString()} µs`;
}