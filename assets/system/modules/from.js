/**
 *  miniRT "from" CommonJS module
 *  (c) 2016 Fat Cerberus
**/

'use strict';
module.exports = from;

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
	else if ('length' in target)
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

function Query(target, asObject)
{
	Object.defineProperties(this,
	{
		all:     PROPDESC('wc', m_makeAdder(MapPoint, AllOp)),
		any:     PROPDESC('wc', m_makeAdder(MapPoint, AnyOp)),
		besides: PROPDESC('wc', m_makeAdder(EachPoint)),
		count:   PROPDESC('wc', m_makeAdder(WherePoint, CountOp)),
		each:    PROPDESC('wc', m_makeAdder(EachPoint, NullOp)),
		first:   PROPDESC('wc', m_makeAdder(TakePoint, SelectOp)),
		map:     PROPDESC('wc', m_makeAdder(MapPoint)),
		remove:  PROPDESC('wc', m_makeAdder(WherePoint, RemoveOp)),
		select:  PROPDESC('wc', m_makeAdder(MapPoint, SelectOp)),
		skip:    PROPDESC('wc', m_makeAdder(SkipPoint)),
		take:    PROPDESC('wc', m_makeAdder(TakePoint)),
		update:  PROPDESC('wc', m_makeAdder(MapPoint, UpdateOp)),
		where:   PROPDESC('wc', m_makeAdder(WherePoint)),
	});

	var m_asObject = !!asObject;
	var m_links = [];
	var m_target = target;

	function m_addLink(link)
	{
		m_links[m_links.length] = link;
	}

	function m_makeAdder(linkType, op)
	{
		if (op !== undefined) {
			return function() {
				m_addLink(Reflect.construct(linkType, arguments));
				var result = m_run(op);
				--m_links.length;
				return result;
			};
		}
		else {
			return function() {
				m_addLink(Reflect.construct(linkType, arguments));
				return this;
			};
		}
	}

	function m_run(opType)
	{
		var op = Reflect.construct(opType, [ m_target, m_asObject ]);
		var numLinks = m_links.length;

		var keys = m_asObject ? Object.keys(m_target) : null;
		var numKeys = m_asObject ? keys.length : m_target.length;
		var env = {};
		for (var i = 0; i < numKeys; ++i) {
			var key = m_asObject ? keys[i] : i;
			env.v = m_target[key];
			env.k = key;
			var accepted = true;
			for (var j = 0; j < numLinks; ++j) {
				if (!(accepted = m_links[j].run(env)))
					break;
			}
			if (accepted && !op.record(env))
				break;
		}
		var output = undefined;
		if ('commit' in op)
			output = op.commit();

		// reset state of all links so the chain can be reused if needed
		for (var i = 0; i < m_links.length; ++i)
			if ('reset' in m_links[i]) m_links[i].reset();

		return output;
	}
}

function EachPoint(fn)
{
	this.fn = fn;

	this.run = function run(env)
	{
		this.fn.call(undefined, env.v, env.k);
		return true;
	};
}

function MapPoint(fn)
{
	this.fn = fn || function(v) { return v; };

	this.run = function run(env)
	{
		env.v = this.fn.call(undefined, env.v, env.k);
		return true;
	};
}

function SkipPoint(count)
{
	var left = Number(count);

	this.reset = function reset()
	{
		left = Number(count);
	};

	this.run = function run(env)
	{
		return left-- <= 0;
	};
}

function TakePoint(count)
{
	var left = Number(count);

	this.reset = function reset()
	{
		left = Number(count);
	};

	this.run = function run(env)
	{
		return left-- > 0;
	};
}

function WherePoint(fn)
{
	this.fn = fn || function() { return true; };

	this.run = function(env)
	{
		return !!this.fn.call(undefined, env.v, env.k);
	};
}

function AllOp(target)
{
	var result = true;

	this.commit = function()
	{
		return result;
	};

	this.record = function(env)
	{
		result = result && !!env.v;
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

	this.record = function(env)
	{
		result = result || !!env.v;
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

	this.record = function(env)
	{
		++numItems;
		return true;
	};
}

function NullOp(target)
{
	this.record = function(env)
	{
		return true;
	};
}

function RemoveOp(target, asObject)
{
	var toRemove = [];

	this.commit = function()
	{
		var doSplice = [].splice.bind(target);
		for (var i = toRemove.length - 1; i >= 0; --i) {
			if (asObject)
				delete target[toRemove[i]];
			else
				doSplice(toRemove[i], 1);
		}
	};

	this.record = function(env)
	{
		toRemove[toRemove.length] = env.k;
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

	this.record = function(env)
	{
		results[index++] = env.v;
		return true;
	};
}

function UpdateOp(target)
{
	this.record = function(env)
	{
		target[env.k] = env.v;
		return true;
	};
}
