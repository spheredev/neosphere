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
	return new Chain(target);
}

function Chain(target)
{
	Object.defineProperties(this,
	{
		all:     PROPDESC('wc', m_makeAdder(MapLink, AllOp)),
		any:     PROPDESC('wc', m_makeAdder(MapLink, AnyOp)),
		besides: PROPDESC('wc', m_makeAdder(ForEachLink)),
		count:   PROPDESC('wc', m_makeAdder(MapLink, CountOp)),
		forEach: PROPDESC('wc', m_makeAdder(ForEachLink, null)),
		map:     PROPDESC('wc', m_makeAdder(MapLink)),
		remove:  PROPDESC('wc', m_makeAdder(MapLink, RemoveOp)),
		select:  PROPDESC('wc', m_makeAdder(MapLink, SelectOp)),
		take:    PROPDESC('wc', m_makeAdder(TakeLink)),
		where:   PROPDESC('wc', m_makeAdder(WhereLink)),
	});

	var m_arrayLike = Array.isArray(target);
	var m_chain = this;
	var m_links = [];
	var m_target = target;

	function m_addLink(link)
	{
		if (m_links.length > 0)
			m_links[m_links.length - 1].next = link;
		m_links.push(link);
		return m_chain;
	}

	function m_makeAdder(linkType, endpoint)
	{
		if (endpoint !== undefined) {
			return function() {
				m_addLink(Reflect.construct(linkType, arguments));
				return m_run(endpoint);
			};
		}
		else {
			return function() {
				return m_addLink(Reflect.construct(linkType, arguments));
			};
		}
	}

	function m_run(opType)
	{
		// we only need the first link.  the rest get called recursively.
		var firstLink = m_links[0];

		// prepare and run the query
		var op = opType !== null
			? Reflect.construct(opType, [ m_target ])
			: { record: function() {} };
		if (m_arrayLike) {
			var numEntries = m_target.length;
			for (var i = 0; i < numEntries; ++i) {
				var result = firstLink.run(m_target[i], i);
				if (result !== null)
					i = op.record(result.v, result.k);
			}
		}
		else {
			var keys = Object.keys(m_target);
			var numEntries = keys.length;
			for (var i = 0; i < numEntries; ++i) {
				var key = keys[i];
				var result = firstLink.run(m_target[key], key);
				if (result !== null)
					op.record(result.v, result.k);
			}
		}

		// remove the final link so the chain can be reused
		--m_links.length;
		var lastLink = m_links[m_links.length - 1];
		delete lastLink.next;
		for (var i = 0; i < m_links.length; ++i)
			('reset' in m_links[i]) && m_links[i].reset();

		return op.value;
	}
}

function ForEachLink(fn)
{
	this.fn = fn;

	this.run = function run(v, k)
	{
		this.fn.call(undefined, v, k);
		if ('next' in this)
			return this.next.run(v, k);
	};
}

function MapLink(fn)
{
	this.fn = fn || function(v) { return v; };

	this.run = function run(v, k)
	{
		v = this.fn.call(undefined, v, k);
		if ('next' in this)
			return this.next.run(v, k);
		else
			return { v: v, k: k };
	};
}

function TakeLink(count)
{
	this.count = Number(count);
	this.left = this.count;

	this.reset = function reset()
	{
		this.left = this.count;
	};

	this.run = function run(v, k)
	{
		return this.left-- > 0
			? this.next.run(v, k)
			: null;
	};
}

function WhereLink(fn)
{
	this.fn = fn || function() { return true; };

	this.run = function(v, k)
	{
		return this.fn.call(undefined, v, k)
			? this.next.run(v, k)
			: null;

	};
}

function AllOp(target)
{
	this.value = true;

	this.record = function(v, k)
	{
		this.value = this.value && !!v;
		return k;
	};
}

function AnyOp(target)
{
	this.value = false;

	this.record = function(v, k)
	{
		this.value = this.value || !!v;
		return k;
	};
}

function CountOp(target)
{
	this.value = 0;

	this.record = function(v, k)
	{
		++this.value;
		return k;
	};
}

function RemoveOp(target)
{
	var arrayLike = Array.isArray(target);

	this.value = target;

	this.record = function(v, k)
	{
		if (arrayLike) {
			target.splice(k, 1);
			return k - 1;
		}
		else {
			delete target[k];
			return k;
		}
	};
}

function SelectOp(target)
{
	var arrayLike = Array.isArray(target);
	var index = 0;

	this.value = arrayLike ? [] : {};

	this.record = function(v, k)
	{
		if (arrayLike)
			this.value[index++] = v;
		else
			this.value[k] = v;
		return k;
	};
}
