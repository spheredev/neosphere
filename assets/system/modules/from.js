/**
 *  miniRT "from" CommonJS module
 *  (c) 2016 Fat Cerberus
**/

'use strict';
module.exports = from;

const assert = require('assert');

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
	target = Object(target);
	if (typeof target.length !== 'number')
		throw new TypeError("object with 'length' required");
	return new Query(target);
}

from.Object = fromObject;
function fromObject(target)
{
	target = Object(target);
	return new Query(target, true);
}

from.String = fromString;
function fromString(target)
{
	if (typeof target !== 'string' && !(target instanceof String))
		throw new TypeError("string or String object required");
	return new Query(target);
}

function Query(target, asObject, memo)
{
	Object.defineProperties(this,
	{
		all:     PROPDESC('wc', m_makePoint(MapPoint, AllOp)),
		allIn:   PROPDESC('wc', m_makePoint(InPoint, AllOp)),
		any:     PROPDESC('wc', m_makePoint(MapPoint, AnyOp)),
		anyIn:   PROPDESC('wc', m_makePoint(InPoint, AnyOp)),
		anyIs:   PROPDESC('wc', m_makePoint(IsPoint, AnyOp)),
		besides: PROPDESC('wc', m_makePoint(EachPoint)),
		count:   PROPDESC('wc', m_makePoint(WherePoint, CountOp)),
		each:    PROPDESC('wc', m_makePoint(EachPoint, NullOp)),
		first:   PROPDESC('wc', m_makePoint(WherePoint, FirstOp)),
		last:    PROPDESC('wc', m_makePoint(WherePoint, LastOp)),
		mapTo:   PROPDESC('wc', m_makePoint(MapPoint)),
		remove:  PROPDESC('wc', m_makePoint(WherePoint, RemoveOp)),
		select:  PROPDESC('wc', m_makePoint(MapPoint, SelectOp)),
		skip:    PROPDESC('wc', m_makePoint(SkipPoint)),
		take:    PROPDESC('wc', m_makePoint(TakePoint)),
		update:  PROPDESC('wc', m_makePoint(MapPoint, UpdateOp)),
		where:   PROPDESC('wc', m_makePoint(WherePoint)),
	});

	var m_asObject = !!asObject;
	var m_points = memo ? memo.points.slice() : [];
	var m_target = target;
	if (memo)
		m_points[m_points.length] = memo.newPoint;

	function m_makePoint(pointType, opType)
	{
		if (opType !== undefined) {
			return function() {
				var point = Reflect.construct(pointType, arguments);
				return m_run(point, opType);
			};
		}
		else {
			return function() {
				return new Query(m_target, m_asObject, {
					points:   m_points,
					newPoint: Reflect.construct(pointType, arguments) });
			};
		}
	}

	function m_run(lastPoint, opType)
	{
		var points = m_points.concat(lastPoint);
		var numLinks = points.length;
		var op = Reflect.construct(opType, [ m_target, m_asObject ]);

		var keys = m_asObject ? Object.keys(m_target) : null;
		var numKeys = m_asObject ? keys.length : m_target.length;
		var item = {};
		for (var i = 0; i < numKeys; ++i) {
			var key = m_asObject ? keys[i] : i;
			item.v = m_target[key];
			item.k = key;
			item.t = m_target;
			var accepted = true;
			for (var j = 0; j < numLinks; ++j) {
				if (!(accepted = points[j].run(item)))
					break;
			}
			if (accepted && !op.record(item))
				break;
		}
		var output = undefined;
		if ('commit' in op)
			output = op.commit();

		// reset state of all links so the chain can be reused if needed
		for (var i = 0; i < points.length; ++i)
			if ('reset' in points[i]) points[i].reset();

		return output;
	}
}

function EachPoint(callback)
{
	callback = callback || function() {};
	assert.equal(typeof callback, 'function');

	this.run = function run(item)
	{
		callback(item.v, item.k, item.t);
		return true;
	};
}

function InPoint(values)
{
	this.run = function run(item)
	{
		var haveMatch = false;
		for (var i = 0; i < values.length; ++i) {
			if (haveMatch = Object.is(item.v, values[i]))
				break;
		}
		item.v = haveMatch;
		return true;
	};
}

function IsPoint(value)
{
	this.run = function run(item)
	{
		item.v = Object.is(item.v, value);
		return true;
	};
}

function MapPoint(selector)
{
	selector = selector || function(v) { return v; };
	assert.equal(typeof selector, 'function');

	this.run = function run(item)
	{
		item.v = selector(item.v, item.k, item.t);
		return true;
	};
}

function SkipPoint(count)
{
	var left = Math.trunc(count);
	assert.ok(left >= 0);

	this.reset = function reset()
	{
		left = Math.trunc(count);
	};

	this.run = function run(item)
	{
		return left-- <= 0;
	};
}

function TakePoint(count)
{
	var left = Math.trunc(count);
	assert.ok(left >= 0);

	this.reset = function reset()
	{
		left = Math.trunc(count);
	};

	this.run = function run(item)
	{
		return left-- > 0;
	};
}

function WherePoint(predicate)
{
	predicate = predicate || function() { return true; };
	assert.equal(typeof predicate, 'function');

	this.run = function(item)
	{
		return !!predicate(item.v, item.k, item.t);
	};
}

function AllOp(target)
{
	var result = true;

	this.commit = function()
	{
		return result;
	};

	this.record = function(item)
	{
		result = result && !!item.v;
		return !!result;
	};
}

function AnyOp(target)
{
	var result = false;

	this.commit = function()
	{
		return result;
	};

	this.record = function(item)
	{
		result = result || !!item.v;
		return !result;
	};
}

function CountOp(target)
{
	var numItems = 0;

	this.commit = function()
	{
		return numItems;
	};

	this.record = function(item)
	{
		++numItems;
		return true;
	};
}

function FirstOp(target)
{
	var m_value;

	this.commit = function()
	{
		return m_value;
	};

	this.record = function(item)
	{
		m_value = item.v;
		return false;
	};
}

function LastOp(target)
{
	var m_value;

	this.commit = function()
	{
		return m_value;
	};

	this.record = function(item)
	{
		m_value = item.v;
		return true;
	};
}

function NullOp(target)
{
	this.record = function(item)
	{
		return true;
	};
}

function RemoveOp(target, asObject)
{
	var keysToRemove = [];

	this.commit = function()
	{
		var doSplice = [].splice.bind(target);
		for (var i = keysToRemove.length - 1; i >= 0; --i) {
			if (asObject)
				delete target[keysToRemove[i]];
			else
				doSplice(keysToRemove[i], 1);
		}
	};

	this.record = function(item)
	{
		keysToRemove[keysToRemove.length] = item.k;
		return true;
	};
}

function SelectOp(target)
{
	var results = [],
	    index = 0;

	this.commit = function()
	{
		return results;
	};

	this.record = function(item)
	{
		results[index++] = item.v;
		return true;
	};
}

function UpdateOp(target)
{
	this.record = function(item)
	{
		item.t[item.k] = item.v;
		return true;
	};
}
