/**
 *  miniRT "from" CommonJS module
 *  (c) 2016 Fat Cerberus
**/

'use strict';
module.exports = from;

const assert = require('assert');
const random = require('random');

function from(target/*, ...*/)
{
	// for multiple targets, query against the list of targets and unroll with
	// .from().
	if (arguments.length > 1)
		return from(arguments).from();

	target = Object(target);
	if (typeof target.length === 'number')
		return fromArray(target);
	else
		return fromObject(target);
};

from.Array = fromArray;
function fromArray(target)
{
	target = Object(target);
	if (typeof target.length !== 'number')
		throw new TypeError("object with 'length' required");
	var itemSource = new ArraySource(target);
	return new FromQuery(itemSource);
}

from.Object = fromObject;
function fromObject(target)
{
	var itemSource = new ObjectSource(Object(target));
	return new FromQuery(itemSource);
}

const PK =
{
	ItemSource: Symbol('itemSource'),
};

function MAKEPOINT(sourceType, op)
{
	// this is some crazy witchcraft, don't try this at home!
	const makeArray = Function.prototype.call.bind(Array.prototype.slice);

	return function(/*...*/)
	{
		// some more crazy witchcraft to emulate ES6 Reflect.construct().
		var source = this[PK.ItemSource];
		var boundArgs = [ null, source ].concat(makeArray(arguments));
		var constructor = Function.prototype.bind.apply(sourceType, boundArgs);
		var newSource = new constructor();

		// if it's a terminal operator, run the query.  otherwise construct
		// a new FromQuery.  this allows LINQ-style composition.
		return op !== undefined ? op(newSource)
			: new FromQuery(newSource);
	};
}

function PROPDESC(flags, valueOrGetter, setter)
{
	var desc = {
		writable:     flags.indexOf('w') !== -1,
		enumerable:   flags.indexOf('e') !== -1,
		configurable: flags.indexOf('c') !== -1,
	};
	if (flags.indexOf('a') === -1)
		desc.value = valueOrGetter;
	else {
		desc.get = valueOrGetter;
		desc.set = setter;
	}
	return desc;
};

function FromQuery(source)
{
	this[PK.ItemSource] = source;
}

Object.defineProperties(FromQuery.prototype,
{
	[Symbol.iterator]:
	PROPDESC('wc', function enumerate()
	{
		var source = this[PK.ItemSource];
		source.init();
		return { next: next };

		function next()
		{
			var item = source.next();
			return item !== null
				? { value: item, done: false }
				: { done: true };
		}
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
	mapTo:      PROPDESC('wc', MAKEPOINT(MapSource)),
	random:     PROPDESC('wc', MAKEPOINT(SampleSource(false))),
	remove:     PROPDESC('wc', MAKEPOINT(FilterSource, removeOp)),
	sample:     PROPDESC('wc', MAKEPOINT(SampleSource(true))),
	select:     PROPDESC('wc', MAKEPOINT(MapSource, collectOp)),
	shuffle:    PROPDESC('wc', MAKEPOINT(ShuffledSource)),
	skip:       PROPDESC('wc', MAKEPOINT(SkipSource)),
	take:       PROPDESC('wc', MAKEPOINT(TakeSource)),
	update:     PROPDESC('wc', MAKEPOINT(MapSource, updateOp)),
	where:      PROPDESC('wc', MAKEPOINT(FilterSource)),
});

function ArraySource(target)
{
	var m_index;
	var m_length;

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
			t: target
		};
	}
}

function ObjectSource(target)
{
	var m_index;
	var m_keys;
	var m_length;

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
		if (m_index >= m_length);
			return null;
		var key = m_keys[m_index++];
		return {
			v: target[key],
			k: key,
			t: target
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
		var item = source.next();
		if (item !== null)
			callback(item.v, item.k, item.t);
		return item;
	};
}

function FilterSource(source, predicate)
{
	predicate = predicate || function(v) { return true; };

	this.init =
	function init()
	{
		source.init();
	};

	this.next =
	function next()
	{
		var item;
		while (item = source.next()) {
			if (predicate(item.v, item.k, item.t))
				break;
		}
		return item;
	};
}

function FromSource(source, selector)
{
	var m_iterator;
	var m_nextItem;
	var m_selector = selector || function(v) { return v; };

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
			var item = source.next();
			if (item !== null) {
				var target = m_selector(item.v, item.k, item.t);
				m_iterator = from(target)[Symbol.iterator]();
				if ((m_nextItem = m_iterator.next()).done)
					m_iterator = null;
			}
			else
				return null;
		}

		var item = m_nextItem.value;
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
		var item = source.next();
		if (item !== null) {
			for (var i = values.length - 1; i >= 0; --i) {
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

function IncludeSource(source, target)
{
	var m_iterator;
	var m_targets = Array.prototype.slice.call(arguments, 1);

	this.init =
	function init()
	{
		source.init();
		m_iterator = from(m_targets).from()[Symbol.iterator]();
	};

	this.next =
	function next()
	{
		var item = source.next();
		if (item === null) {
			var result;
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
		var item = source.next();
		if (item !== null)
			item.v = item.v === value;
		return item;
	};
}

function MapSource(source, selector)
{
	selector = selector || function(v) { return v; };

	this.init =
	function init()
	{
		source.init();
	};

	this.next =
	function next()
	{
		var item = source.next();
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
	return function(source, keySelector)
	{
		var m_index = 0;
		var m_length;
		var m_list = [];

		this.init =
		function init()
		{
			var index;
			var item;
			var order = descending ? -1 : 1;

			// this is kind of ugly pulling all results in advance, but there's
			// not much we can do about it here.  we could sort as we go, but
			// since the rest of the query can't do anything until the results
			// are fully sorted anyway, that doesn't buy us much.
			source.init();
			while (item = source.next()) {
				index = m_list.length;
				m_list[index] = {
					index: index,  // to stabilize the sort
					item:  item,
					key:   keySelector(item.v, item.k, item.t)
				};
			}

			// Array#sort() is not guaranteed to be stable.  to stabilize it,
			// we use the item's position in the input stream as a tiebreaker.
			m_list.sort(function(a, b) {
				return a.key < b.key ? -1 * order
					: a.key > b.key ? 1 * order
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

function SampleSource(uniqueOnly)
{
	return function(source, count)
	{
		var m_count = Number(count);
		var m_items = [];
		var m_numSamples;

		this.init =
		function init()
		{
			var item;

			source.init();
			while (item = source.next())
				m_items[m_items.length] = item;
			m_numSamples = 0;
		};

		this.next =
		function next()
		{
			var index;
			var item;

			if (m_numSamples++ < m_count) {
				index = random.discrete(0, m_items.length - 1);
				item = m_items[index];
				if (uniqueOnly)
					m_items.splice(index, 1);
				return item;
			}
			else
				return null;
		};
	};
}

function ShuffledSource(source)
{
	var m_index = 0;
	var m_length;
	var m_list = [];

	this.init =
	function init()
	{
		var index;
		var item;
		var temp;

		// as with the sorting operators, Fisher-Yates shuffle doesn't really
		// lend itself to streaming so we just pull everything in advance.
		source.init();
		while (item = source.next()) {
			index = m_list.length;
			m_list[index] = item;
		}
		for (var i = m_list.length - 1; i >= 1; --i) {
			index = random.discrete(0, i);
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
	var m_count = Number(count);
	var m_numSkipped;

	this.init =
	function init()
	{
		source.init();
		m_numSkipped = 0;
	};

	this.next =
	function next()
	{
		var item;

		item = m_numSkipped++ >= m_count
			? source.next() : null;
		return item;
	};
}

function TakeSource(source, count)
{
	var m_count = Number(count);
	var m_numTaken;

	this.init =
	function init()
	{
		source.init();
		m_numTaken = 0;
	};

	this.next =
	function next()
	{
		var item;

		item = m_numTaken++ < m_count
			? source.next() : null;
		return item;
	};
}

function allOp(source)
{
	var item;

	source.init();
	while (item = source.next()) {
		if (!item.v)
			return false;
	}
	return true;
};

function anyOp(source)
{
	var item;

	source.init();
	while (item = source.next()) {
		if (!!item.v)
			return true;
	}
	return false;
};

function collectOp(source)
{
	var collection = [];
	var item;

	source.init();
	while (item = source.next())
		collection[collection.length] = item.v;
	return collection;
};

function countOp(source)
{
	var numItems = 0;

	source.init();
	while (source.next())
		++numItems;
	return numItems;
};

function firstOp(source)
{
	var item;

	source.init();
	if (item = source.next())
		return item.v;
};

function lastOp(source)
{
	var lastResult;
	var item;

	source.init();
	while (item = source.next()) {
		lastResult = item.v;
	}
	return lastResult;
};

function nullOp(source)
{
	source.init();
	while (source.next()) {
		/* no-op */
	}
};

function removeOp(source)
{
	var item;
	var splice = Array.prototype.splice;
	var toRemove = [];

	source.init();
	while (item = source.next())
		toRemove[toRemove.length] = { k: item.k, t: item.t };
	for (var i = toRemove.length - 1; i >= 0; --i) {
		item = toRemove[i];
		if (typeof item.t.length === 'number')
			splice.call(item.t, item.k, 1);
		else
			delete item.t[item.k]
	}
};

function updateOp(source)
{
	var item;

	source.init();
	while (item = source.next())
		item.t[item.k] = item.v;
};
