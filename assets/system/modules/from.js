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
	if ('length' in target)
		return fromArray(target);
	else
		return fromObject(target);
};

from.Array = fromArray;
function fromArray(target)
{
	target = Object(target);
	if (typeof target.length !== 'number')
		throw new TypeError("object has no 'length'");
	return new Query(target);
}

from.Object = fromObject;
function fromObject(target)
{
	target = Object(target);
	return new Query(target, true);
}

function Query(target, asObject)
{
	Object.defineProperties(this,
	{
		all:     PROPDESC('wc', m_makeAdder(MapLink, AllOp)),
		any:     PROPDESC('wc', m_makeAdder(MapLink, AnyOp)),
		besides: PROPDESC('wc', m_makeAdder(ForEachLink)),
		count:   PROPDESC('wc', m_makeAdder(WhereLink, CountOp)),
		first:   PROPDESC('wc', m_makeAdder(TakeLink, SelectOp)),
		forEach: PROPDESC('wc', m_makeAdder(ForEachLink, NullOp)),
		map:     PROPDESC('wc', m_makeAdder(MapLink)),
		remove:  PROPDESC('wc', m_makeAdder(WhereLink, RemoveOp)),
		select:  PROPDESC('wc', m_makeAdder(MapLink, SelectOp)),
		skip:    PROPDESC('wc', m_makeAdder(SkipLink)),
		take:    PROPDESC('wc', m_makeAdder(TakeLink)),
		update:  PROPDESC('wc', m_makeAdder(MapLink, UpdateOp)),
		where:   PROPDESC('wc', m_makeAdder(WhereLink)),
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
		if ('commit' in op)
			op.commit();

		// reset state of all links so the chain can be reused if needed
		for (var i = 0; i < m_links.length; ++i)
			if ('reset' in m_links[i]) m_links[i].reset();

		return op.value;
	}
}

function ForEachLink(fn)
{
	this.fn = fn;

	this.run = function run(env)
	{
		this.fn.call(undefined, env.v, env.k);
		return true;
	};
}

function MapLink(fn)
{
	this.fn = fn || function(v) { return v; };

	this.run = function run(env)
	{
		env.v = this.fn.call(undefined, env.v, env.k);
		return true;
	};
}

function SkipLink(count)
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

function TakeLink(count)
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

function WhereLink(fn)
{
	this.fn = fn || function() { return true; };

	this.run = function(env)
	{
		return !!this.fn.call(undefined, env.v, env.k);
	};
}

function AllOp(target)
{
	this.value = true;

	this.record = function(env)
	{
		this.value = this.value && !!env.v;
		return this.value;
	};
}

function AnyOp(target)
{
	this.value = false;

	this.record = function(env)
	{
		this.value = this.value || !!env.v;
		return !this.value;
	};
}

function CountOp(target)
{
	this.value = 0;

	this.record = function(env)
	{
		++this.value;
		return true;
	};
}

function FirstOp(target)
{
	var numWanted = 1;

	this.value = [];

	this.commit = function()
	{
		if (numWanted === 1)
			this.value = this.value[0];
	};

	this.record = function(env)
	{
		this.value[this.value.length] = env.v;
		return this.value.length < numWanted;
	};
}

function NullOp(target)
{
	this.value = undefined;

	this.record = function(env)
	{
		return true;
	};
}

function RemoveOp(target, asObject)
{
	var toRemove = [];

	this.value = target;

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
	var index = 0;

	this.value = [];

	this.record = function(env)
	{
		this.value[index++] = env.v;
		return true;
	};
}

function UpdateOp(target)
{
	this.value = target;

	this.record = function(env)
	{
		target[env.k] = env.v;
		return true;
	};
}
