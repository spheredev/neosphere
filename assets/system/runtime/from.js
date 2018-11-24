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
			const next = i + 1 < len ? this.opcodes[i + 1] : null;
			if (opcode.type === 'reverse' && next !== null && next.type === 'reverse') {
				// optimize away consecutive 'reverse' opcodes
				++i;
				continue;
			}
			const op = opcode.type === 'drop' ? new DropOp(opcode.a)
				: opcode.type === 'filter' ? new FilterOp(opcode.a)
				: opcode.type === 'map' ? new MapOp(opcode.a)
				: opcode.type === 'plus' ? new PlusOp(opcode.a)
				: opcode.type === 'over' ? new OverOp(opcode.a)
				: opcode.type === 'reverse' ? new ReverseOp()
				: opcode.type === 'minus' ? new MinusOp(opcode.a)
				: opcode.type === 'take' ? new TakeOp(opcode.a)
				: opcode.type === 'thru' ? new ThruOp(opcode.a)
				: opcode.type === 'uniq' ? new UniqOp(opcode.a)
				: undefined;
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
			if (firstOp !== null)
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
		const is = value !== value ? Object.is : (x, y) => x === y;
		return this.any(it => is(it, value));
	}

	ascending(keymaker = IdentityFunction)
	{
		return this.thru(all => {
			const pairs = all.map(it => [ keymaker(it), it ]);
			pairs.sort((a, b) => a[0] < b[0] ? -1 : b[0] < a[0] ? +1 : 0);
			return pairs.map(it => it[1]);
		});
	}

	besides(iteratee)
	{
		return this.addOp$('map', (it, k) => (iteratee(it, k), it));
	}

	count(keymaker)
	{
		return this.reduce((a, it) => {
			if (keymaker !== undefined) {
				if (a === null)
					a = Object.create(null);
				const key = keymaker(it);
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

	descending(keymaker = IdentityFunction)
	{
		return this.thru(all => {
			const pairs = all.map(it => [ keymaker(it), it ]);
			pairs.sort((b, a) => a[0] < b[0] ? -1 : b[0] < a[0] ? +1 : 0);
			return pairs.map(it => it[1]);
		});
	}

	drop(count)
	{
		return this.addOp$('drop', count);
	}

	find(predicate)
	{
		return this.run$(new FindOp((it, key, memo) => predicate(it) ? (memo.value = it, true) : false));
	}

	findIndex(predicate)
	{
		return this.run$(new FindOp((it, key, memo) => predicate(it) ? (memo.value = key, true) : false));
	}

	first(mapper)
	{
		return this.run$(new FindOp((it, key, memo) => (memo.value = mapper ? mapper(it) : it, true)));
	}

	forEach(iteratee)
	{
		this.reduce((a, it) => iteratee(it));
	}

	last(mapper)
	{
		return this.run$(new LastOp(mapper));
	}

	over(mapper)
	{
		return this.addOp$('over', mapper);
	}

	plus(...sources)
	{
		return this.addOp$('plus', sources);
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
		return this.addOp$('reverse');
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

	select(mapper)
	{
		return this.addOp$('map', mapper);
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
		return this.addOp$('take', count);
	}

	tap(callback)
	{
		return this.thru(all => (callback(all), all));
	}

	thru(mapper)
	{
		return this.addOp$('thru', mapper);
	}

	toArray()
	{
		return this.run$(new ToArrayOp());
	}

	uniq(keymaker = IdentityFunction)
	{
		return this.addOp$('uniq', keymaker);
	}

	update(mapper)
	{
		return this.run$(new UpdateOp(mapper));
	}

	where(predicate)
	{
		return this.addOp$('filter', predicate);
	}

	without(...values)
	{
		return this.addOp$('minus', values);
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

	push(value, source, key)
	{
		if (this.nextOp !== undefined)
			this.nextOp.push(value, source, key);
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

class FilterOp extends QueryOp
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

class LastOp extends QueryOp
{
	constructor(mapper)
	{
		super();
		this.mapper = mapper;
	}

	initialize()
	{
		this.haveItem = false;
		this.result = undefined;
	}

	flush()
	{
		if (this.mapper !== undefined && this.haveItem)
			this.result = this.mapper(this.result);
	}

	push(value)
	{
		this.haveItem = true;
		this.result = value;
	}
}

class MapOp extends QueryOp
{
	constructor(mapper)
	{
		super();
		this.mapper = mapper;
	}

	push(value, source, key)
	{
		value = this.mapper(value, key);
		return this.nextOp.push(value, source, key);
	}
}

class MinusOp extends QueryOp
{
	constructor(sources)
	{
		super();
		this.values = [].concat(...sources);
	}

	push(value, source, key)
	{
		if (!this.values.includes(value))
			return this.nextOp.push(value, source, key);
		return true;
	}
}

class OverOp extends QueryOp
{
	constructor(mapper)
	{
		super();
		this.mapper = mapper;
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
		const sublist = this.mapper(value);
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

class UpdateOp extends QueryOp
{
	constructor(mapper)
	{
		super();
		this.mapper = mapper;
	}

	initialize(sources)
	{
		if (sources === undefined)
			throw new TypeError("update() cannot be used with transformations");
	}

	push(value, source, key)
	{
		source[key] = this.mapper(value, key);
		return true;
	}
}

class UniqOp extends QueryOp
{
	constructor(keymaker)
	{
		super();
		this.keymaker = keymaker;
	}

	initialize()
	{
		this.keys = [];
		super.initialize();
	}

	push(value, source, key)
	{
		const uniqKey = this.keymaker(value);
		if (!this.keys.includes(uniqKey)) {
			this.keys.push(uniqKey);
			return this.nextOp.push(value, source, key);
		}
		return true;
	}
}
