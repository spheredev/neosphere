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
		this.outputBordered = false;
		this.placeholder = new Record();
		if (this.enabled)
			this.exitJob = Dispatch.onExit(() => this.finish());
	}

	finish()
	{
		if (!this.enabled)
			return; //in case someone manually calls this
		
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
			[ "Calls" ],
			[ "Time (us)" ],
			[ "% Total" ],
			[ "Avg (us)" ],
			[ "% Avg" ],
		];
		for (const record of this.records) {
			if (record.count > 0) {
				consoleOutput[0].push(record.description);
				consoleOutput[1].push(record.count.toLocaleString());
				consoleOutput[2].push(Math.round(record.totalTime / 1000).toLocaleString());
				consoleOutput[3].push(Math.round(100 * record.totalTime / totalTime) + "%");
				consoleOutput[4].push(Math.round(record.averageTime / 1000).toLocaleString());
				consoleOutput[5].push(Math.round(100 * record.averageTime / totalAverage) + "%");
			}
			if (record.attached === true)
				record.target[record.methodName] = record.originalFunction;
		}
		consoleOutput[0].push("Total");
		consoleOutput[1].push(calls.toLocaleString());
		consoleOutput[2].push(Math.round(totalTime / 1000).toLocaleString());
		consoleOutput[3].push("100%");
		consoleOutput[4].push(Math.round(totalAverage / 1000).toLocaleString());
		consoleOutput[5].push("100%");


		SSj.log(`Profiling results for "${Sphere.Game.name}"\n` + makeTable(consoleOutput, this.outputBordered));
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
			throw new Error("Attempt to profile re-entrant block - this is not supported");
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

	profile(target, methodName, description)
	{
		if (!this.enabled)
			return;

		let originalFunction = target[methodName];
		let record = new Record(`${target.constructor.name}#${methodName}`,
			true, methodName, originalFunction, target);

		if (typeof description === 'string')
			record.description = description;

		target[methodName] = function (...args) {
			let startTime = now();
			let result = originalFunction.apply(this, args);
			record.totalTime += now() - startTime;
			++record.count;
			return result;
		};
		this.records.push(record);
	}
};

//constructor function used for Record objects to limit risk of introducing multiple types
//don't want profiler to accidently trigger a Jit bailout (relevant as these can be created
//in two places)
//ES5 style constructor as simple/short and class construction is slightly slower in CC
function Record (description, attached, methodName, originalFunction, target)
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

function makeTable(table, bordered)
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
				column[j] = " " + column[j].padStart(width) + " ";
			else
				column[j] = column[j].padEnd(width) + " ";
			if (bordered === true)
				column[j] += "|";
		}
		totalLength += (width + 3);
	}

	let start = "\n";
	let line = "";
	let startLine = "";
	if (bordered === true) {
		start = "\n|";
		line = "\n" + ("-".repeat(totalLength - 1));
	}
	else {
		startLine = "\n" + ("-".repeat(totalLength - 1));
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
