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
function from(...targets)
{
	// for multiple targets, query against the list of targets and unroll with
	// .from().
	if (targets.length > 1)
		return from(targets).from();

	let target = Object(targets[0]);
	if (typeof target.length === 'number')
		return fromArray(target);
	else if (typeof target[Symbol.iterator] === 'function' || typeof target.next === 'function')
		return fromIterable(target);
	else
		return fromObject(target);
}

from.array = fromArray;
function fromArray(target)
{
	target = Object(target);
	if (typeof target.length !== 'number')
		throw new TypeError("object with 'length' required");
	let itemSource = new ArraySource(target);
	return new FromQuery(itemSource);
}

from.iterable = fromIterable;
function fromIterable(target)
{
	target = Object(target);
	if (typeof target[Symbol.iterator] !== 'function' && typeof target.next !== 'function')
		throw new TypeError("object is not iterable");
	let itemSource = new IterableSource(target);
	return new FromQuery(itemSource);
}

from.object = fromObject;
function fromObject(target)
{
	let itemSource = new ObjectSource(Object(target));
	return new FromQuery(itemSource);
}

function MAKEPOINT(sourceType, op)
{
	return function (...args) {
		let thisSource = this.itemSource;
		let newSource = new sourceType(thisSource, ...args);

		// if it's a terminal operator, run the query.  otherwise construct
		// a new FromQuery.  this allows LINQ-style composition.
		return op !== undefined ? op(newSource)
			: new FromQuery(newSource);
	};
}

function PROPDESC(flags, valueOrGetter, setter)
{
	let desc = {
		writable:     flags.indexOf('w') !== -1,
		enumerable:   flags.indexOf('e') !== -1,
		configurable: flags.indexOf('c') !== -1,
	};
	if (flags.indexOf('a') === -1) {
		desc.value = valueOrGetter;
	}
	else {
		desc.get = valueOrGetter;
		desc.set = setter;
	}
	return desc;
}

function FromQuery(source)
{
	this.itemSource = source;
}

Object.defineProperties(FromQuery.prototype, {
	[Symbol.iterator]:
	PROPDESC('wc', function* enumerate(withKeys)
	{
		let source = this.itemSource;
		let item;
		source.init();
		while ((item = source.next()))
			yield withKeys ? item : item.v;
	}),

	// from() query operators.
	// refer to the miniRT API reference to find out what each of these does.
	all:        PROPDESC('wc', MAKEPOINT(MapSource, allOp)),
	allIn:      PROPDESC('wc', MAKEPOINT(InSource, allOp)),
	any:        PROPDESC('wc', MAKEPOINT(MapSource, anyOp)),
	anyIn:      PROPDESC('wc', MAKEPOINT(InSource, anyOp)),
	anyIs:      PROPDESC('wc', MAKEPOINT(IsSource, anyOp)),
	ascending:  PROPDESC('wc', MAKEPOINT(OrderedSource(false))),
	besides:    PROPDESC('wc', MAKEPOINT(CallbackSource)),
	count:      PROPDESC('wc', MAKEPOINT(FilterSource, countOp)),
	descending: PROPDESC('wc', MAKEPOINT(OrderedSource(true))),
	each:       PROPDESC('wc', MAKEPOINT(CallbackSource, nullOp)),
	first:      PROPDESC('wc', MAKEPOINT(FilterSource, firstOp)),
	from:       PROPDESC('wc', MAKEPOINT(FromSource)),
	including:  PROPDESC('wc', MAKEPOINT(IncludeSource)),
	last:       PROPDESC('wc', MAKEPOINT(FilterSource, lastOp)),
	random:     PROPDESC('wc', MAKEPOINT(SampleSource(false))),
	reduce:     PROPDESC('wc', MAKEPOINT(ReduceSource, firstOp)),
	remove:     PROPDESC('wc', MAKEPOINT(FilterSource, removeOp)),
	sample:     PROPDESC('wc', MAKEPOINT(SampleSource(true))),
	select:     PROPDESC('wc', MAKEPOINT(MapSource)),
	shuffle:    PROPDESC('wc', MAKEPOINT(ShuffledSource)),
	skip:       PROPDESC('wc', MAKEPOINT(SkipSource)),
	take:       PROPDESC('wc', MAKEPOINT(TakeSource)),
	toArray:    PROPDESC('wc', MAKEPOINT(MapSource, collectOp)),
	update:     PROPDESC('wc', MAKEPOINT(MapSource, updateOp)),
	where:      PROPDESC('wc', MAKEPOINT(FilterSource)),
});

function ArraySource(target)
{
	let m_index;
	let m_length;

	this.init =
	function init()
	{
		m_index = 0;
		m_length = target.length;
	};

	this.next =
	function next()
	{
		if (m_index >= m_length)
			return null;
		return {
			v: target[m_index],
			k: m_index++,
			t: target,
		};
	};
}

function IterableSource(target)
{
	let m_iter;

	this.init =
	function init()
	{
		if (typeof target[Symbol.iterator] === 'function')
			m_iter = target[Symbol.iterator]();
		else
			m_iter = target;
	};

	this.next =
	function next()
	{
		let result = m_iter.next();
		if (result.done)
			return null;
		return {
			v: result.value,
			k: null,
			t: target,
		};
	};
}

function ObjectSource(target)
{
	let m_index;
	let m_keys;
	let m_length;

	this.init =
	function init()
	{
		m_keys = Object.keys(target);
		m_index = 0;
		m_length = m_keys.length;
	};

	this.next =
	function next()
	{
		if (m_index >= m_length)
			return null;
		let key = m_keys[m_index++];
		return {
			v: target[key],
			k: key,
			t: target,
		};
	};
}

function CallbackSource(source, callback)
{
	this.init =
	function init()
	{
		source.init();
	};

	this.next =
	function next()
	{
		let item = source.next();
		if (item !== null)
			callback(item.v, item.k, item.t);
		return item;
	};
}

function FilterSource(source, predicate)
{
	predicate = predicate || (() => true);

	this.init =
	function init()
	{
		source.init();
	};

	this.next =
	function next()
	{
		let item;
		while ((item = source.next())) {
			if (predicate(item.v, item.k, item.t))
				break;
		}
		return item;
	};
}

function FromSource(source, selector)
{
	let m_iterator;
	let m_nextItem;
	let m_selector = selector || (it => it);

	this.init =
	function init()
	{
		source.init();
		m_iterator = null;
	};

	this.next =
	function next()
	{
		while (m_iterator === null) {
			let item = source.next();
			if (item !== null) {
				let target = m_selector(item.v, item.k, item.t);
				m_iterator = from(target)[Symbol.iterator](true);
				if ((m_nextItem = m_iterator.next()).done)
					m_iterator = null;
			}
			else {
				return null;
			}
		}

		let item = m_nextItem.value;
		if ((m_nextItem = m_iterator.next()).done)
			m_iterator = null;
		return item;
	};
}

function InSource(source, values)
{
	this.init =
	function init()
	{
		source.init();
	};

	this.next =
	function next()
	{
		let item = source.next();
		if (item !== null) {
			for (let i = values.length - 1; i >= 0; --i) {
				if (item.v === values[i]) {
					item.v = true;
					return item;
				}
			}
			item.v = false;
		}
		return item;
	};
}

function IncludeSource(source, ...targets)
{
	let m_iterator;

	this.init =
	function init()
	{
		source.init();
		m_iterator = from(targets).from()[Symbol.iterator](true);
	};

	this.next =
	function next()
	{
		let item = source.next();
		if (item === null) {
			let result;
			if (!(result = m_iterator.next()).done)
				item = result.value;
		}
		return item;
	};
}

function IsSource(source, value)
{
	this.init =
	function init()
	{
		source.init();
	};

	this.next =
	function next()
	{
		let item = source.next();
		if (item !== null)
			item.v = item.v === value;
		return item;
	};
}

function MapSource(source, selector)
{
	selector = selector || (it => it);

	this.init =
	function init()
	{
		source.init();
	};

	this.next =
	function next()
	{
		let item = source.next();
		if (item !== null)
			item.v = selector(item.v, item.k, item.t);
		return item;
	};
}

// this uses pigsort, which is awesome.  pigsort is based on the idea that if
// you put a bunch of big fat eaty pigs in close proximity (i.e. within a
// hundred-mile radius of each other) and leave them to their devices for a
// while, the whole menagerie of pigs will have invariably come to resemble a
// Matryoshka doll by the time you decide to come back.  so basically it's like
// bubble sort, but with eaty pigs.
function OrderedSource(descending)
{
	return function (source, keySelector) {
		let m_index = 0;
		let m_length;
		let m_list = [];

		this.init =
		function init()
		{
			let index;
			let item;
			let order = descending ? -1 : 1;

			// this is kind of ugly pulling all results in advance, but there's
			// not much we can do about it here.  we could sort as we go, but
			// since the rest of the query can't do anything until the results
			// are fully sorted anyway, that doesn't buy us much.
			source.init();
			while ((item = source.next())) {
				index = m_list.length;
				m_list[index] = {
					index: index,  // to stabilize the sort
					item:  item,
					key:   keySelector(item.v, item.k, item.t),
				};
			}

			// Array#sort() is not guaranteed to be stable.  to stabilize it,
			// we use the item's position in the input stream as a tiebreaker.
			m_list.sort((a, b) => {
				return a.key < b.key ? -1 * order
					: a.key > b.key ? +1 * order
					: a.index - b.index;
			});
			m_index = 0;
			m_length = m_list.length;
		};

		this.next =
		function next()
		{
			if (m_index < m_length)
				return m_list[m_index++].item;
			else
				return null;
		};
	};
}

function ReduceSource(source, reducer, initialValue)
{
	let m_sentValue = false;
	let m_value = undefined;

	this.init =
	function init()
	{
		let item;

		// as with the sorting operators, Fisher-Yates shuffle doesn't really
		// lend itself to streaming so we just pull everything in advance.
		source.init();
		if (initialValue !== undefined) {
			m_value = initialValue;
		}
		else {
			if ((item = source.next()))
				m_value = item.v;
			else
				return;
		}
		while ((item = source.next()))
			m_value = reducer(m_value, item.v, item.k, item.t);
	};

	this.next =
	function next()
	{
		if (!m_sentValue) {
			m_sentValue = true;
			return { v: m_value };
		}
		else {
			return null;
		}
	};
}

function SampleSource(uniqueOnly)
{
	return function (source, count) {
		let m_count = Number(count);
		let m_items = [];
		let m_numSamples;

		this.init =
		function init()
		{
			let item;

			source.init();
			while ((item = source.next()))
				m_items[m_items.length] = item;
			m_numSamples = 0;
		};

		this.next =
		function next()
		{
			let index;
			let item;

			if (m_numSamples++ < m_count) {
				index = Math.floor(Math.random() * m_items.length);
				item = m_items[index];
				if (uniqueOnly)
					m_items.splice(index, 1);
				return item;
			}
			else {
				return null;
			}
		};
	};
}

function ShuffledSource(source)
{
	let m_index = 0;
	let m_length;
	let m_list = [];

	this.init =
	function init()
	{
		let index;
		let item;
		let temp;

		// as with the sorting operators, Fisher-Yates shuffle doesn't really
		// lend itself to streaming so we just pull everything in advance.
		source.init();
		while ((item = source.next())) {
			index = m_list.length;
			m_list[index] = item;
		}
		for (let i = m_list.length - 1; i >= 1; --i) {
			index = Math.floor(Math.random() * i);
			temp = m_list[index];
			m_list[index] = m_list[i];
			m_list[i] = temp;
		}
		m_index = 0;
		m_length = m_list.length;
	};

	this.next =
	function next()
	{
		if (m_index < m_length)
			return m_list[m_index++];
		else
			return null;
	};
}

function SkipSource(source, count)
{
	let m_count = Number(count);
	let m_numSkipped;

	this.init =
	function init()
	{
		source.init();
		m_numSkipped = 0;
	};

	this.next =
	function next()
	{
		let item;

		item = m_numSkipped++ >= m_count
			? source.next() : null;
		return item;
	};
}

function TakeSource(source, count)
{
	let m_count = Number(count);
	let m_numTaken;

	this.init =
	function init()
	{
		source.init();
		m_numTaken = 0;
	};

	this.next =
	function next()
	{
		let item;

		item = m_numTaken++ < m_count
			? source.next() : null;
		return item;
	};
}

function allOp(source)
{
	let item;

	source.init();
	while ((item = source.next())) {
		if (!item.v)
			return false;
	}
	return true;
}

function anyOp(source)
{
	let item;

	source.init();
	while ((item = source.next())) {
		if (item.v)
			return true;
	}
	return false;
}

function collectOp(source)
{
	let collection = [];
	let item;

	source.init();
	while ((item = source.next()))
		collection[collection.length] = item.v;
	return collection;
}

function countOp(source)
{
	let numItems = 0;

	source.init();
	while (source.next())
		++numItems;
	return numItems;
}

function firstOp(source)
{
	let item;

	source.init();
	if ((item = source.next()))
		return item.v;
}

function lastOp(source)
{
	let lastResult;
	let item;

	source.init();
	while ((item = source.next()))
		lastResult = item.v;
	return lastResult;
}

function nullOp(source)
{
	source.init();
	while (source.next()) {
		/* no-op */
	}
}

function removeOp(source)
{
	let item;
	let splice = Array.prototype.splice;
	let toRemove = [];

	source.init();
	while ((item = source.next()))
		toRemove[toRemove.length] = { k: item.k, t: item.t };
	for (let i = toRemove.length - 1; i >= 0; --i) {
		item = toRemove[i];
		if (typeof item.t.length === 'number')
			splice.call(item.t, item.k, 1);
		else
			delete item.t[item.k];
	}
}

function updateOp(source)
{
	let item;

	source.init();
	while ((item = source.next()))
		item.t[item.k] = item.v;
}
