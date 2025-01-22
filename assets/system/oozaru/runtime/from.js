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

export default
function from(...sources)
{
	if (sources.some(it => it === null || it === undefined))
		throw new TypeError("Query source is null or undefined");
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
			const op = new opcode.type(opcode.a, opcode.b);
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
			if (sources.some(it => it === null || it === undefined))
				throw new TypeError("Query source is null or undefined");
			if (lastOp !== null)
				lastOp.nextOp = reduceOp;
			else
				firstOp = reduceOp;
			firstOp.initialize(sources);
			for (let i = 0, len = sources.length; i < len; ++i) {
				const source = sources[i];
				if (!feedMeSeymour(firstOp, source))
					break;
			}
			return firstOp.flush(sources);
		};
		return this.sources !== undefined
			? runQuery(...this.sources)
			: runQuery;
	}

	aggregate(reducer, seedValue)
	{
		return this.run$(new AggregateOp(reducer, seedValue));
	}

	all(predicate)
	{
		return this.run$(new FindOp((it, key, memo) => !predicate(it, key) ? (memo.value = false, true) : false, true));
	}

	allIn(values)
	{
		const valueSet = new Set(values);
		return this.all(it => valueSet.has(it));
	}

	any(predicate)
	{
		return this.run$(new FindOp((it, key, memo) => predicate(it, key) ? (memo.value = true, true) : false, false));
	}

	anyIn(values)
	{
		const valueSet = new Set(values);
		return this.any(it => valueSet.has(it));
	}

	anyIs(value)
	{
		const match = value !== value ? x => x !== x : x => x === value;
		return this.any(it => match(it));
	}

	apply(values)
	{
		return this.selectMany(fn => from(values).select(fn));
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

	call(...args)
	{
		return this.select(it => it(...args)).toArray();
	}

	concat(...sources)
	{
		return this.addOp$(ConcatOp, sources);
	}

	count()
	{
		return this.aggregate(n => n + 1, 0);
	}

	countBy(keySelector)
	{
		return this.aggregate((a, it) => {
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

	distinct(keySelector = identity)
	{
		return this.addOp$(DistinctOp, keySelector);
	}

	elementAt(index)
	{
		return this.skip(index).first();
	}

	first(predicate = always)
	{
		return this.run$(new FindOp((it, key, memo) => predicate(it) ? (memo.value = it, true) : false));
	}

	forEach(iteratee)
	{
		this.aggregate((a, it) => iteratee(it));
	}

	groupBy(keySelector)
	{
		return this.run$(new GroupOp(keySelector));
	}

	join(collection, predicate, selector)
	{
		return this.selectMany(outer =>
			from(collection)
				.where(it => predicate(outer, it))
				.select(it => selector(outer, it)));
	}

	last(predicate = always)
	{
		return this.run$(new LastOp(predicate));
	}

	memoize()
	{
		return from(this.toArray());
	}

	plus(...values)
	{
		return this.addOp$(ConcatOp, [ values ]);
	}

	pull(...values)
	{
		const valueSet = new Set(values);
		return this.remove(it => valueSet.has(it));
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

	selectMany(selector)
	{
		return this.addOp$(SelectManyOp, selector);
	}

	shuffle()
	{
		return this.thru(all => {
			for (let i = 0, len = all.length; i < len - 1; ++i) {
				const pick = i + Math.floor(Math.random() * (len - i));
				const value = all[pick];
				all[pick] = all[i];
				all[i] = value;
			}
			return all;
		});
	}

	single(predicate = always)
	{
		return this.run$(new SingleOp(predicate));
	}

	skip(count)
	{
		return this.addOp$(SkipOp, count);
	}

	skipLast(count)
	{
		return this.addOp$(SkipLastOp, count);
	}

	skipWhile(predicate)
	{
		return this.addOp$(SkipWhileOp, predicate);
	}

	take(count)
	{
		return this.addOp$(TakeOp, count);
	}

	takeLast(count)
	{
		// takeLast can't be lazily evaluated because we don't know where to
		// start until we've seen the final result.
		return this.thru(values => (count > 0 ? values.slice(-count) : []));
	}

	takeWhile(predicate)
	{
		return this.addOp$(TakeWhileOp, predicate);
	}

	thru(transformer)
	{
		return this.addOp$(ThruOp, transformer);
	}

	toArray()
	{
		return this.run$(new ToArrayOp());
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

	zip(collection, selector = tupleify)
	{
		return this.addOp$(ZipOp, collection, selector);
	}
}

class QueryOp
{
	initialize(sources)
	{
		// `initialize()` is called at the start of query execution, before the
		// first item is sent to `step()`.
		if (this.nextOp !== undefined)
			this.nextOp.initialize(sources);
	}

	flush(sources)
	{
		// `flush()` is called after all items from the source have been sent
		// into the pipeline.  this provides for operations that need to see
		// all results before they can do their work, e.g. sorting.  the
		// aggregator at the end of the chain should return its final result
		// from `flush()`.
		return this.nextOp !== undefined
			? this.nextOp.flush(sources)
			: undefined;
	}

	step(value, source, key)
	{
		// `step()` should return `true` to continue execution, or `false` to
		// short-circuit.  note that `flush()` will still be called even if the
		// query short-circuits.
		return this.nextOp !== undefined
			? this.nextOp.step(value, source, key)
			: false;
	}
}

class ThruOp extends QueryOp
{
	constructor(transformer)
	{
		super();
		this.transformer = transformer;
	}

	initialize()
	{
		this.values = [];
		super.initialize();
	}

	flush()
	{
		const newSource = this.transformer(this.values);
		if (this.nextOp instanceof ThruOp) {
			// if the next operator is a ThruOp, just give it our buffer since
			// we don't need it anymore.  this greatly speeds up queries with
			// multiple consecutive ThruOps.
			this.nextOp.values = Array.isArray(newSource)
				? newSource
				: Array.from(newSource);
		}
		else {
			feedMeSeymour(this.nextOp, newSource);
		}
		return this.nextOp.flush();
	}

	step(value)
	{
		this.values.push(value);
		return true;
	}
}

class AggregateOp extends QueryOp
{
	constructor(aggregator, seedValue)
	{
		super();
		this.aggregator = aggregator;
		this.seedValue = seedValue;
	}

	initialize()
	{
		this.accumulator = this.seedValue;
		super.initialize();
	}

	flush()
	{
		return this.accumulator;
	}

	step(value, source, key)
	{
		this.accumulator = this.aggregator(this.accumulator, value, key);
		return true;
	}
}

class ConcatOp extends QueryOp
{
	constructor(sources)
	{
		super();
		this.sources = sources;
	}

	flush()
	{
		for (let i = 0, len = this.sources.length; i < len; ++i) {
			if (!feedMeSeymour(this.nextOp, this.sources[i]))
				break;
		}
		return this.nextOp.flush();
	}

	step(value, source, key)
	{
		return this.nextOp.step(value, source, key);
	}
}

class DistinctOp extends QueryOp
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

class FindOp extends QueryOp
{
	constructor(finder, defaultValue)
	{
		super();
		this.defaultValue = defaultValue;
		this.finder = finder;
	}

	initialize(sources)
	{
		this.memo = { value: this.defaultValue };
	}

	flush(sources)
	{
		return this.memo.value;
	}

	step(value, source, key)
	{
		// if the `finder` returns true, short-circuit the query.
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
		this.groupMap = new Map();
	}

	flush()
	{
		const groups = {};
		for (const [ key, list ] of this.groupMap.entries())
			groups[key] = list;
		return groups;
	}

	step(value)
	{
		const key = this.keySelector(value);
		let list = this.groupMap.get(key);
		if (list === undefined)
			this.groupMap.set(key, list = []);
		list.push(value);
		return true;
	}
}

class LastOp extends QueryOp
{
	constructor(predicate)
	{
		super();
		this.predicate = predicate;
	}

	initialize()
	{
		this.lastValue = undefined;
	}

	flush()
	{
		return this.lastValue;
	}

	step(value, source, key)
	{
		if (this.predicate(value, key))
			this.lastValue = value;
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
					if (r < this.removals.length && i === this.removals[r].key) {
						// note: array length is adjusted after the loop.
						++r;
						continue;
					}
					source[j++] = source[i];
				}
				source.length = j;
			}
			else {
				for (let i = 0, len = this.removals.length; i < len; ++i) {
					if (this.removals[i].source === source)
						delete source[this.removals[i].key];
				}
			}
		}
	}

	step(value, source, key)
	{
		if (this.predicate === undefined || this.predicate(value, key))
			this.removals.push({ source, key });
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
			if (this.nextOp instanceof SkipOp) {
				start -= this.nextOp.left;
				this.nextOp.left = 0;
			}
			for (let i = start; i >= 0; --i) {
				if (!this.nextOp.step(this.values[i], this.values, i))
					break;
			}
		}
		return this.nextOp.flush();
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
		const newValue = this.selector(value, key);
		return this.nextOp.step(newValue, source, key);
	}
}

class SelectManyOp extends QueryOp
{
	constructor(selector)
	{
		super();
		this.selector = selector;
	}

	initialize()
	{
		// don't pass the sources through.  OverOp is not implemented as a
		// ThruOp to avoid the creation of a temp array but it's still a
		// transformative operation so we don't want to allow use of remove()
		// or update() after this.
		super.initialize();
	}

	step(value, source, key)
	{
		const itemSource = this.selector(value, key);
		return feedMeSeymour(this.nextOp, itemSource);
	}
}

class SingleOp extends QueryOp
{
	constructor(predicate)
	{
		super();
		this.predicate = predicate;
	}

	initialize()
	{
		this.lastValue = undefined;
		this.count = 0;
	}

	flush()
	{
		return this.lastValue;
	}

	step(value, source, key)
	{
		if (this.predicate(value, key)) {
			if (++this.count > 1)
				throw new Error("Query would return too many results");
			this.lastValue = value;
		}
		return true;
	}
}

class SkipOp extends QueryOp
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
		return this.left-- <= 0
			? this.nextOp.step(value, source, key)
			: true;
	}
}

class SkipLastOp extends QueryOp
{
	constructor(count)
	{
		super();
		this.count = count;
	}

	initialize(sources)
	{
		this.buffer = new Array(this.count);
		this.ptr = 0;
		this.left = this.count;
		super.initialize(sources);
	}

	step(value, source, key)
	{
		const nextUp = this.buffer[this.ptr];
		this.buffer[this.ptr] = { value, key, source };
		this.ptr = (this.ptr + 1) % this.count;
		return this.left-- <= 0
			? this.nextOp.step(nextUp.value, nextUp.source, nextUp.key)
			: true;
	}
}

class SkipWhileOp extends QueryOp
{
	constructor(predicate)
	{
		super();
		this.predicate = predicate;
	}

	initialize(sources)
	{
		this.onTheTake = false;
		super.initialize(sources);
	}

	step(value, source, key)
	{
		if (!this.onTheTake)
			this.onTheTake = !this.predicate(value, key);
		return this.onTheTake
			? this.nextOp.step(value, source, key)
			: true;
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

class TakeWhileOp extends QueryOp
{
	constructor(predicate)
	{
		super();
		this.predicate = predicate;
	}

	initialize(sources)
	{
		this.onTheTake = true;
		super.initialize(sources);
	}

	step(value, source, key)
	{
		if (this.onTheTake)
			this.onTheTake = this.predicate(value, key);
		return this.onTheTake
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
		return this.values;
	}

	step(value)
	{
		this.values.push(value);
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
		return this.predicate(value, key)
			? this.nextOp.step(value, source, key)
			: true;
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
		return (!this.values.has(value))
			? this.nextOp.step(value, source, key)
			: true;
	}
}

class ZipOp extends QueryOp
{
	constructor(source, selector)
	{
		super();
		this.source = source;
		this.selector = selector;
	}

	initialize(sources)
	{
		this.iterator = this.source[Symbol.iterator]();
		super.initialize(sources);
	}

	step(value, source, key)
	{
		const iterResult = this.iterator.next();
		if (iterResult.done)
			return false;
		const newValue = this.selector(value, iterResult.value, key);
		return this.nextOp.step(newValue, source, key);
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

function feedMeSeymour(queryOp, source)
{
	if (isArrayLike(source)) {
		let start = 0;
		if (queryOp instanceof SkipOp) {
			start = queryOp.left;
			queryOp.left -= source.length;
			if (queryOp.left < 0)
				queryOp.left = 0;
		}
		for (let i = start, len = source.length; i < len; ++i) {
			const value = source[i];
			if (!queryOp.step(value, source, i))
				return false;
		}
	}
	else if (isIterable(source)) {
		for (const value of source) {
			if (!queryOp.step(value, source))
				return false;
		}
	}
	else if (source instanceof Query) {
		if (!source.all((it, k, s) => queryOp.step(it, s, k)))
			return false;
	}
	else {
		const keys = Object.keys(source);
		let start = 0;
		if (queryOp instanceof SkipOp) {
			start = queryOp.left;
			queryOp.left -= source.length;
			if (queryOp.left < 0)
				queryOp.left = 0;
		}
		for (let i = start, len = keys.length; i < len; ++i) {
			const value = source[keys[i]];
			if (!queryOp.step(value, source, keys[i]))
				return false;
		}
	}
	return true;
}

function always()
{
	return true;
}

function identity(value)
{
	return value;
}

function tupleify(lValue, rValue)
{
	return [ lValue, rValue ];
}
