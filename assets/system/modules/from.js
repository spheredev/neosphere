/**
 *  miniRT "from" CommonJS module
 *  (c) 2016 Fat Cerberus
**/

'use strict';
module.exports = from;

const assert = require('assert');
const random = require('random');

function PROPDESC(flags, value)
{
	var desc = {};

	return {
		writable:     flags.indexOf('w') !== -1,
		enumerable:   flags.indexOf('e') !== -1,
		configurable: flags.indexOf('c') !== -1,
		value:        value
	};
};

function from(target)
{
	target = Object(target);
	if (typeof target === 'string' || target instanceof String)
		return fromString(target);
	else if (typeof target.length === 'number')
		return fromArray(target);
	else
		return fromObject(target);
};

from.Array = fromArray;
function fromArray(target)
{
	var source;

	target = Object(target);
	if (typeof target.length !== 'number')
		throw new TypeError("object with 'length' required");
	source = new ArraySource(target);
	return new Queryable(source);
}

from.Object = fromObject;
function fromObject(target)
{
	var source;

	source = new ObjectSource(Object(target));
	return new Queryable(source);
}

from.String = fromString;
function fromString(target)
{
	var source;

	if (typeof target !== 'string' && !(target instanceof String))
		throw new TypeError("string or String object required");
	source = new ArraySource(target);
	return new Queryable(source);
}

function Queryable(source)
{
	Object.defineProperties(this,
	{
		all:        PROPDESC('wc', m_makePoint(MapSource, allOp)),
		allIn:      PROPDESC('wc', m_makePoint(InSource, allOp)),
		any:        PROPDESC('wc', m_makePoint(MapSource, anyOp)),
		anyIn:      PROPDESC('wc', m_makePoint(InSource, anyOp)),
		anyIs:      PROPDESC('wc', m_makePoint(IsSource, anyOp)),
		ascending:  PROPDESC('wc', m_makePoint(AscendingSource)),
		besides:    PROPDESC('wc', m_makePoint(CallbackSource)),
		count:      PROPDESC('wc', m_makePoint(FilterSource, countOp)),
		descending: PROPDESC('wc', m_makePoint(DescendingSource)),
		each:       PROPDESC('wc', m_makePoint(CallbackSource, nullOp)),
		first:      PROPDESC('wc', m_makePoint(FilterSource, firstOp)),
		from:       PROPDESC('wc', m_makePoint(FromSource)),
		last:       PROPDESC('wc', m_makePoint(FilterSource, lastOp)),
		mapTo:      PROPDESC('wc', m_makePoint(MapSource)),
		random:     PROPDESC('wc', m_makePoint(RandomSource, collectOp)),
		remove:     PROPDESC('wc', m_makePoint(FilterSource, removeOp)),
		sample:     PROPDESC('wc', m_makePoint(SampleSource, collectOp)),
		select:     PROPDESC('wc', m_makePoint(MapSource, collectOp)),
		shuffle:    PROPDESC('wc', m_makePoint(ShuffledSource)),
		skip:       PROPDESC('wc', m_makePoint(SkipSource)),
		take:       PROPDESC('wc', m_makePoint(TakeSource)),
		update:     PROPDESC('wc', m_makePoint(MapSource, updateOp)),
		where:      PROPDESC('wc', m_makePoint(FilterSource)),
	});

	var m_source = source;

	function m_makePoint(sourceType, op)
	{
		return function()
		{
			var constructArgs = [ m_source ]
				.concat([].slice.call(arguments));
			var source = Reflect.construct(sourceType, constructArgs);
			return op !== undefined
				? op(source)
				: new Queryable(source);
		};
	}
}

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
		var key;

		if (m_index >= m_length);
			return null;
		key = m_keys[m_index++];
		return {
			v: target[key],
			k: key,
			t: target
		};
	};
}

function AscendingSource(source, keySelector)
{
	var m_index = 0;
	var m_length;
	var m_ordered = [];

	this.init =
	function init()
	{
		var index;
		var item;

		source.init();
		while (item = source.next()) {
			index = m_ordered.length;
			m_ordered[index] = {
				index: index,  // to stabilize the sort
				item:  item,
				key:   keySelector(item.v, item.k, item.t)
			};
		}
		m_ordered.sort(function(a, b) {
			return a.key < b.key ? -1
				: a.key > b.key ? 1
				: a.index - b.index;
		});
		m_index = 0;
		m_length = m_ordered.length;
	};

	this.next =
	function next()
	{
		if (m_index < m_length)
			return m_ordered[m_index++].item;
		else
			return null;
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
		var item;

		if (item = source.next())
			callback(item.v, item.k, item.t);
		return item;
	};
}

function DescendingSource(source, keySelector)
{
	var m_index = 0;
	var m_length;
	var m_ordered = [];

	this.init =
	function init()
	{
		var index;
		var item;

		source.init();
		while (item = source.next()) {
			index = m_ordered.length;
			m_ordered[index] = {
				index: index,  // to stabilize the sort
				item:  item,
				key:   keySelector(item.v, item.k, item.t)
			};
		}
		m_ordered.sort(function(b, a) {
			return a.key < b.key ? -1
				: a.key > b.key ? 1
				: a.index - b.index;
		});
		m_index = 0;
		m_length = m_ordered.length;
	};

	this.next =
	function next()
	{
		if (m_index < m_length)
			return m_ordered[m_index++].item;
		else
			return null;
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
	var m_index;
	var m_items = null;
	var m_selector = selector || function(v) { return v; };

	this.init =
	function init()
	{
		source.init();
		m_list = null;
	};

	this.next =
	function next()
	{
		var item;
		var target;

		if (m_items == null) {
			if (item = source.next()) {
				target = m_selector(item.v, item.k, item.t);
				m_index = 0;
				m_items = from(target).select(function(v, k, t) {
					return { v: v, k: k, t: t };
				});
			}
			else
				return null;
		}

		item = m_items[m_index++];
		if (m_index >= m_list.length)
			m_items = null;
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
		var item;

		if (item = source.next()) {
			for (var i = values.length - 1; i >= 0; --i) {
				if (Object.is(item.v, values[i])) {
					item.v = true;
					return;
				}
			}
			item.v = false;
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
		var item;

		if (item = source.next())
			item.v = Object.is(item.v, value);
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
		var item;

		if (item = source.next())
			item.v = selector(item.v, item.k, item.t);
		return item;
	};
}

function RandomSource(source, count)
{
	var m_count = Number(count);
	var m_numSamples;
	var m_items = [];

	this.init =
	function init()
	{
		var item;

		source.init();
		while (item = source.next())
			m_items[m_items.length] = item;
		m_numSampled = 0;
	};

	this.next =
	function next()
	{
		if (m_numSamples++ < m_count)
			return random.sample(m_items);
		else
			return null;
	};
}

function SampleSource(source, count)
{
	var m_count = Number(count);
	var m_numSamples;
	var m_items = [];

	this.init =
	function init()
	{
		var item;

		source.init();
		while (item = source.next())
			m_items[m_items.length] = item;
		m_numSampled = 0;
	};

	this.next =
	function next()
	{
		var index;
		var item;

		if (m_numSamples++ < m_count) {
			index = random.discrete(0, m_items.length);
			item = m_items[index];
			m_items.splice(index, 0);
			return item;
		}
		else
			return null;
	};
}

function ShuffledSource(source)
{
	var m_index = 0;
	var m_length;
	var m_shuffled = [];

	this.init =
	function init()
	{
		var index;
		var item;
		var temp;

		source.init();
		while (item = source.next()) {
			index = m_shuffled.length;
			m_shuffled[index] = item;
		}
		for (var i = m_shuffled.length - 1; i >= 1; --i) {
			index = random.discrete(0, i);
			temp = m_shuffled[index];
			m_shuffled[index] = m_shuffled[i];
			m_shuffled[i] = temp;
		}
		m_index = 0;
		m_length = m_shuffled.length;
	};

	this.next =
	function next()
	{
		if (m_index < m_length)
			return m_shuffled[m_index++];
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
		if ('length' in item.t)
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
