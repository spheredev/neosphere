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
				if (isArrayLike(source)) {
					let start = 0;
					if (firstOp instanceof DropOp) {
						start = firstOp.left;
						firstOp.left -= source.length;
						if (firstOp.left < 0)
							firstOp.left = 0;
					}
					for (let i = start, len = source.length; i < len; ++i) {
						const value = source[i];
						if (!firstOp.step(value, source, i))
							break outer_loop;
					}
				}
				else if (isIterable(source)) {
					for (const value of source) {
						if (!firstOp.step(value, source))
							break outer_loop;
					}
				}
				else if (source instanceof Query) {
					if (!source.all((it, k, s) => firstOp.step(it, s, k)))
						break outer_loop;
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
						if (!firstOp.step(value, source, keys[i]))
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

	applyTo(values)
	{
		return this.over(f => from(values).select(f));
	}

	ascending(keySelector = identity)
	{
		return this.thru(all => {
			const pairs = all.map(it => ({ key: keySelector(it), value: it }));
			pairs.sort((a, b) => a.key < b.key ? -1 : b.key < a.key ? +1 : 0);
			return pairs.map(it => it.value);
		});
	}

	besides(iteratee)
	{
		return this.select((it, k) => (iteratee(it, k), it));
	}

	count()
	{
		return this.reduce(n => n + 1, 0);
	}

	countBy(keySelector)
	{
		return this.reduce((a, it) => {
			const key = keySelector(it);
			if (a[key] !== undefined)
				++a[key];
			else
				a[key] = 1;
			return a;
		}, Object.create(null));
	}

	descending(keySelector = identity)
	{
		return this.thru(all => {
			const pairs = all.map(it => ({ key: keySelector(it), value: it }));
			pairs.sort((b, a) => a.key < b.key ? -1 : b.key < a.key ? +1 : 0);
			return pairs.map(it => it.value);
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

	plus(...values)
	{
		return this.addOp$(PlusOp, values);
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

	thru(replacer)
	{
		return this.addOp$(ThruOp, replacer);
	}

	toArray()
	{
		return this.run$(new ToArrayOp());
	}

	uniq(keySelector = identity)
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
		return this.addOp$(WithoutOp, values);
	}
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

	step(value, source, key)
	{
		if (this.nextOp !== undefined)
			this.nextOp.step(value, source, key);
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
		if (!Array.isArray(newList) && isIterable(newList))
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
				if (!this.nextOp.step(newList[i], newList, i))
					break;
			}
		}
		super.flush();
	}

	step(value)
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

	step(value, source, key)
	{
		if (this.left-- <= 0)
			this.nextOp.step(value, source, key);
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

	flush(sources)
	{
		this.result = this.memo.value;
		super.flush(sources);
	}

	step(value, source, key)
	{
		return !this.finder(value, key, this.memo);
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

	step(value)
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

	step(value)
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

	step(value)
	{
		const sublist = this.selector(value);
		if (isArrayLike(sublist)) {
			for (let i = 0, len = sublist.length; i < len; ++i) {
				if (!this.nextOp.step(sublist[i], sublist, i))
					return false;
			}
		}
		else if (isIterable(sublist)) {
			for (const value of sublist) {
				if (!this.nextOp.step(value, sublist))
					return false;
			}
		}
		else if (sublist instanceof Query) {
			return sublist.all((it, k, s) => this.nextOp.step(it, s, k));
		}
		return true;
	}
}

class PlusOp extends QueryOp
{
	constructor(values)
	{
		super();
		this.values = values;
	}

	flush()
	{
		for (let i = 0, len = this.values.length; i < len; ++i) {
			if (!this.nextOp.step(this.values[i]))
				break;
		}
		super.flush();
	}

	step(value, source, key)
	{
		return this.nextOp.step(value, source, key);
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

	step(value)
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
			if (isArrayLike(source)) {
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

	step(value, source, key)
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
				if (!this.nextOp.step(this.values[i], this.values, i))
					break;
			}
		}
		this.nextOp.flush();
	}

	step(value)
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

	step(value, source, key)
	{
		value = this.selector(value, key);
		return this.nextOp.step(value, source, key);
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

	step(value, source, key)
	{
		return this.left-- > 0
			? this.nextOp.step(value, source, key)
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

	step(value)
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

	step(value, source, key)
	{
		const uniqKey = this.keySelector(value);
		if (!this.keys.has(uniqKey)) {
			this.keys.add(uniqKey);
			return this.nextOp.step(value, source, key);
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

	step(value, source, key)
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

	step(value, source, key)
	{
		if (this.predicate(value, key))
			return this.nextOp.step(value, source, key);
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

	step(value, source, key)
	{
		if (!this.values.has(value))
			return this.nextOp.step(value, source, key);
		return true;
	}
}

function isArrayLike(value)
{
	// note: strings have a numeric `.length`, but aren't usually treated as
	//       collections, so this check is purposely designed to exclude them.
	return value !== null && typeof value === 'object'
		&& typeof value.length === 'number';
}

function isIterable(value)
{
	return value !== null && value !== undefined
		&& typeof value[Symbol.iterator] === 'function';
}

function identity(value)
{
	return value;
}
