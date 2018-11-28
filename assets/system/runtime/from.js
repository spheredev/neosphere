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

const IdentityFunction = x => x;

export default
function from(...sources)
{
	let query = new Query();
	query.sources = sources;
	return query;
}

export
class Query
{
	get [Symbol.toStringTag]() { return 'Query'; }

	constructor()
	{
		this.opcodes = [];
		this.firstOp = null;
		this.lastOp = null;
	}

	[Symbol.iterator]()
	{
		return this.toArray()[Symbol.iterator]();
	}

	addOp$(type, a, b)
	{
		const opcode = { type, a, b };
		const newQuery = new Query();
		newQuery.sources = this.sources;
		newQuery.opcodes = [ ...this.opcodes, opcode ];
		return newQuery;
	}

	compile$()
	{
		if (this.firstOp !== null)
			return;
		this.firstOp = null;
		this.lastOp = null;
		for (let i = 0, len = this.opcodes.length; i < len; ++i) {
			const opcode = this.opcodes[i];
			const op = new opcode.type(opcode.a);
			if (this.lastOp !== null)
				this.lastOp.nextOp = op;
			this.lastOp = op;
			if (this.firstOp === null)
				this.firstOp = this.lastOp;
		}
		return this.firstOp;
	}

	run$(reduceOp)
	{
		this.compile$();
		let firstOp = this.firstOp;
		let lastOp = this.lastOp;
		const runQuery = (...sources) => {
			if (lastOp !== null)
				lastOp.nextOp = reduceOp;
			else
				firstOp = reduceOp;
			firstOp.initialize(sources);
			outer_loop: for (let i = 0, len = sources.length; i < len; ++i) {
				const source = sources[i];
				if (typeof source.length === 'number') {
					let start = 0;
					if (firstOp instanceof DropOp) {
						start = firstOp.left;
						firstOp.left -= source.length;
						if (firstOp.left < 0)
							firstOp.left = 0;
					}
					for (let i = start, len = source.length; i < len; ++i) {
						const value = source[i];
						if (!firstOp.push(value, source, i))
							break outer_loop;
					}
				}
				else if (typeof source[Symbol.iterator] === 'function') {
					for (const value of source) {
						if (!firstOp.push(value, source))
							break outer_loop;
					}
				}
				else {
					const keys = Object.keys(source);
					let start = 0;
					if (firstOp instanceof DropOp) {
						start = firstOp.left;
						firstOp.left -= source.length;
						if (firstOp.left < 0)
							firstOp.left = 0;
					}
					for (let i = start, len = keys.length; i < len; ++i) {
						const value = source[keys[i]];
						if (!firstOp.push(value, source, keys[i]))
							break outer_loop;
					}
				}
			}
			firstOp.flush(sources);
			return reduceOp.result;
		};
		return this.sources !== undefined
			? runQuery(...this.sources)
			: runQuery;
	}

	all(predicate)
	{
		return this.run$(new FindOp((it, key, memo) => !predicate(it) ? (memo.value = false, true) : false, true));
	}

	allIn(values)
	{
		return this.all(it => values.includes(it));
	}

	any(predicate)
	{
		return this.run$(new FindOp((it, key, memo) => predicate(it) ? (memo.value = true, true) : false, false));
	}

	anyIn(values)
	{
		return this.any(it => values.includes(it));
	}

	anyIs(value)
	{
		const match = value !== value ? x => x !== x : x => x === value;
		return this.any(it => match(it));
	}

	ascending(keySelector = IdentityFunction)
	{
		return this.thru(all => {
			const pairs = all.map(it => [ keySelector(it), it ]);
			pairs.sort((a, b) => a[0] < b[0] ? -1 : b[0] < a[0] ? +1 : 0);
			return pairs.map(it => it[1]);
		});
	}

	besides(iteratee)
	{
		return this.select((it, k) => (iteratee(it, k), it));
	}

	count(keySelector)
	{
		return this.reduce((a, it) => {
			if (keySelector !== undefined) {
				if (a === null)
					a = Object.create(null);
				const key = keySelector(it);
				if (a[key] !== undefined)
					++a[key];
				else
					a[key] = 1;
			}
			else {
				if (a === null)
					a = 0;
				++a;
			}
			return a;
		}, null);
	}

	descending(keySelector = IdentityFunction)
	{
		return this.thru(all => {
			const pairs = all.map(it => [ keySelector(it), it ]);
			pairs.sort((b, a) => a[0] < b[0] ? -1 : b[0] < a[0] ? +1 : 0);
			return pairs.map(it => it[1]);
		});
	}

	drop(count)
	{
		return this.addOp$(DropOp, count);
	}

	find(predicate)
	{
		return this.run$(new FindOp((it, key, memo) => predicate(it) ? (memo.value = it, true) : false));
	}

	findIndex(predicate)
	{
		return this.run$(new FindOp((it, key, memo) => predicate(it) ? (memo.value = key, true) : false));
	}

	first(selector)
	{
		return this.run$(new FindOp((it, key, memo) => (memo.value = selector ? selector(it) : it, true)));
	}

	forEach(iteratee)
	{
		this.reduce((a, it) => iteratee(it));
	}

	groupBy(keySelector)
	{
		return this.run$(new GroupOp(keySelector));
	}

	last(selector)
	{
		return this.run$(new LastOp(selector));
	}

	over(selector)
	{
		return this.addOp$(OverOp, selector);
	}

	plus(...sources)
	{
		return this.addOp$(PlusOp, sources);
	}

	random(count)
	{
		return this.thru(all => {
			let samples = [];
			for (let i = 0, len = all.length; i < count; ++i) {
				const index = Math.floor(Math.random() * len);
				samples.push(all[index]);
			}
			return samples;
		});
	}

	reduce(reducer, initialValue)
	{
		return this.run$(new ReduceOp(reducer, initialValue));
	}

	remove(predicate)
	{
		return this.run$(new RemoveOp(predicate));
	}

	reverse()
	{
		return this.addOp$(ReverseOp);
	}

	sample(count)
	{
		return this.thru(all => {
			const nSamples = Math.min(Math.max(count, 0), all.length);
			for (let i = 0, len = all.length; i < nSamples; ++i) {
				const pick = i + Math.floor(Math.random() * (len - i));
				const value = all[pick];
				all[pick] = all[i];
				all[i] = value;
			}
			all.length = nSamples;
			return all;
		});
	}

	select(selector)
	{
		return this.addOp$(SelectOp, selector);
	}

	shuffle()
	{
		return this.thru(all => {
			for (let i = 0, len = all.length - 1; i < len; ++i) {
				const pick = i + Math.floor(Math.random() * (len - i));
				const value = all[pick];
				all[pick] = all[i];
				all[i] = value;
			}
			return all;
		});
	}

	take(count)
	{
		return this.addOp$(TakeOp, count);
	}

	tap(callback)
	{
		return this.thru(all => (callback(all), all));
	}

	thru(mapper)
	{
		return this.addOp$(ThruOp, mapper);
	}

	toArray()
	{
		return this.run$(new ToArrayOp());
	}

	uniq(keySelector = IdentityFunction)
	{
		return this.addOp$(UniqOp, keySelector);
	}

	update(selector)
	{
		return this.run$(new UpdateOp(selector));
	}

	where(predicate)
	{
		return this.addOp$(WhereOp, predicate);
	}

	without(...values)
	{
		return this.addOp$(WithoutOp, flatten(values));
	}
}

function flatten(array)
{
	const flattened = [];
	let j = 0;
	for (let i = 0, len = array.length; i < len; ++i) {
		const item = array[i];
		if (typeof item === 'object' && typeof item.length === 'number') {
			for (let i = 0, len = item.length; i < len; ++i)
				flattened[j++] = item[i];
		}
		else {
			flattened[j++] = item;
		}
	}
	return flattened;
}

class QueryOp
{
	flush(sources)
	{
		if (this.nextOp !== undefined)
			this.nextOp.flush(sources);
	}

	initialize(sources)
	{
		if (this.nextOp !== undefined)
			this.nextOp.initialize(sources);
	}

	push(value, source, key)
	{
		if (this.nextOp !== undefined)
			this.nextOp.push(value, source, key);
	}
}

class ThruOp extends QueryOp
{
	constructor(mapper)
	{
		super();
		this.mapper = mapper;
	}

	initialize()
	{
		this.values = [];
		super.initialize();
	}

	flush()
	{
		let newList = this.mapper(this.values);
		if (typeof newList.length !== 'number')
			newList = [ ...newList ];
		if (this.nextOp instanceof ThruOp) {
			// if the next query op is a ThruOp, just give it our buffer since we don't need it anymore.
			// this greatly speeds up queries with multiple consecutive ThruOps.
			this.nextOp.values = newList;
		}
		else {
			let start = 0;
			if (this.nextOp instanceof DropOp) {
				start = this.nextOp.left;
				this.nextOp.left = 0;
			}
			for (let i = start, len = newList.length; i < len; ++i) {
				if (!this.nextOp.push(newList[i], newList, i))
					break;
			}
		}
		super.flush();
	}

	push(value)
	{
		this.values.push(value);
		return true;
	}
}

class DropOp extends QueryOp
{
	constructor(count)
	{
		super();
		this.count = count;
	}

	initialize(sources)
	{
		this.left = this.count;
		super.initialize(sources);
	}

	push(value, source, key)
	{
		if (this.left-- <= 0)
			this.nextOp.push(value, source, key);
		return true;
	}
}

class FindOp extends QueryOp
{
	constructor(finder, defaultValue)
	{
		super();
		this.defaultValue = defaultValue;
		this.finder = finder;
	}

	initialize(source)
	{
		this.memo = { value: this.defaultValue };
		super.initialize(source);
	}

	push(value, source, key)
	{
		if (this.finder(value, key, this.memo)) {
			this.result = this.memo.value;
			return false;
		}
		return true;
	}
}

class GroupOp extends QueryOp
{
	constructor(keySelector)
	{
		super();
		this.keySelector = keySelector;
	}

	initialize()
	{
		this.result = new Map();
	}

	flush()
	{
		const mapEntries = this.result.entries();
		this.result = {};
		for (const entry of mapEntries)
			this.result[entry[0]] = entry[1];
	}

	push(value)
	{
		const key = this.keySelector(value);
		let list = this.result.get(key);
		if (list === undefined)
			this.result.set(key, list = []);
		list.push(value);
		return true;
	}
}

class LastOp extends QueryOp
{
	constructor(selector)
	{
		super();
		this.selector = selector;
	}

	initialize()
	{
		this.haveItem = false;
		this.result = undefined;
	}

	flush()
	{
		if (this.selector !== undefined && this.haveItem)
			this.result = this.selector(this.result);
	}

	push(value)
	{
		this.haveItem = true;
		this.result = value;
	}
}

class OverOp extends QueryOp
{
	constructor(selector)
	{
		super();
		this.selector = selector;
	}

	initialize()
	{
		// don't pass the sources through.  OverOp is not implemented as a ThruOp to avoid the
		// creation of a temp array but it's still a transformative operation so we don't want
		// to allow use of remove() or update() after this.
		super.initialize();
	}

	push(value)
	{
		const sublist = this.selector(value);
		for (let i = 0, len = sublist.length; i < len; ++i) {
			if (!this.nextOp.push(sublist[i], sublist, i))
				return false;
		}
		return true;
	}
}

class PlusOp extends QueryOp
{
	constructor(sources)
	{
		super();
		this.sources = sources;
	}

	flush()
	{
		source_loop:
		for (let i = 0, len = this.sources.length; i < len; ++i) {
			const source = this.sources[i];
			const isObject = typeof source === 'object';
			if (isObject && typeof source.length === 'number') {
				for (let i = 0, len = source.length; i < len; ++i) {
					if (!this.nextOp.push(source[i], source, i))
						break source_loop;
				}
			}
			else if (isObject && Symbol.iterator in source) {
				for (const value of source) {
					if (!this.nextOp.push(value, source))
						break source_loop;
				}
			}
			else {
				if (!this.nextOp.push(source))
					break;
			}
		}
		super.flush();
	}

	push(value, source, key)
	{
		return this.nextOp.push(value, source, key);
	}
}

class ReduceOp extends QueryOp
{
	constructor(reducer, initialValue)
	{
		super();
		this.initialValue = initialValue;
		this.reducer = reducer;
	}

	initialize()
	{
		this.result = this.initialValue;
		super.initialize();
	}

	push(value)
	{
		this.result = this.reducer(this.result, value);
		return true;
	}
}

class RemoveOp extends QueryOp
{
	constructor(predicate)
	{
		super();
		this.predicate = predicate;
	}

	initialize(sources)
	{
		if (sources === undefined)
			throw new TypeError("remove() cannot be used with transformations");
		this.removals = [];
	}

	flush(sources)
	{
		let r = 0;
		for (let m = 0, len = sources.length; m < len; ++m) {
			const source = sources[m];
			if (typeof source.length === 'number') {
				let j = 0;
				for (let i = 0, len = source.length; i < len; ++i) {
					if (i !== this.removals[r][1])
						source[j++] = source[i];
					else
						r++;
				}
				source.length = j;
			}
			else {
				for (let i = 0, len = this.removals.length; i < len; ++i) {
					if (this.removals[i][0] === source)
						delete source[this.removals[i][1]];
				}
			}
		}
	}

	push(value, source, key)
	{
		if (this.predicate === undefined || this.predicate(value, key))
			this.removals.push([ source, key ]);
		return true;
	}
}

class ReverseOp extends ThruOp
{
	constructor()
	{
		super();
	}

	initialize()
	{
		this.values = [];
		super.initialize();
	}

	flush()
	{
		if (this.nextOp instanceof ThruOp) {
			this.values.reverse();
			this.nextOp.values = this.values;
		}
		else {
			const length = this.values.length;
			let start = length - 1;
			if (this.nextOp instanceof DropOp) {
				start -= this.nextOp.left;
				this.nextOp.left = 0;
			}
			for (let i = start; i >= 0; --i) {
				if (!this.nextOp.push(this.values[i], this.values, i))
					break;
			}
		}
		this.nextOp.flush();
	}

	push(value)
	{
		this.values.push(value);
		return true;
	}
}

class SelectOp extends QueryOp
{
	constructor(selector)
	{
		super();
		this.selector = selector;
	}

	push(value, source, key)
	{
		value = this.selector(value, key);
		return this.nextOp.push(value, source, key);
	}
}

class TakeOp extends QueryOp
{
	constructor(count)
	{
		super();
		this.count = count;
	}

	initialize(sources)
	{
		this.left = this.count;
		super.initialize(sources);
	}

	push(value, source, key)
	{
		return this.left-- > 0
			? this.nextOp.push(value, source, key)
			: false;
	}
}

class ToArrayOp extends ThruOp
{
	constructor()
	{
		super();
	}

	initialize()
	{
		this.values = [];
		super.initialize();
	}

	flush()
	{
		this.result = this.values;
	}

	push(value)
	{
		this.values.push(value);
		return true;
	}
}

class UniqOp extends QueryOp
{
	constructor(keySelector)
	{
		super();
		this.keySelector = keySelector;
	}

	initialize()
	{
		this.keys = new Set();
		super.initialize();
	}

	push(value, source, key)
	{
		const uniqKey = this.keySelector(value);
		if (!this.keys.has(uniqKey)) {
			this.keys.add(uniqKey);
			return this.nextOp.push(value, source, key);
		}
		return true;
	}
}

class UpdateOp extends QueryOp
{
	constructor(selector)
	{
		super();
		this.selector = selector;
	}

	initialize(sources)
	{
		if (sources === undefined)
			throw new TypeError("update() cannot be used with transformations");
	}

	push(value, source, key)
	{
		source[key] = this.selector(value, key);
		return true;
	}
}

class WhereOp extends QueryOp
{
	constructor(predicate)
	{
		super();
		this.predicate = predicate;
	}

	push(value, source, key)
	{
		if (this.predicate(value, key))
			return this.nextOp.push(value, source, key);
		return true;
	}
}

class WithoutOp extends QueryOp
{
	constructor(values)
	{
		super();
		this.values = new Set(values);
	}

	push(value, source, key)
	{
		if (!this.values.has(value))
			return this.nextOp.push(value, source, key);
		return true;
	}
}
