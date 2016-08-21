/**
 *  miniRT assert CommonJS module
 *  (c) 2015-2016 Fat Cerberus
**/

'use strict';
module.exports =
{
	deepEqual:			deepEqual,
	deepStrictEqual:	deepStrictEqual,
	equal:				equal,
	fail:				fail,
	notDeepEqual:		notDeepEqual,
	notDeepStrictEqual: notDeepStrictEqual,
	notEqual:			notEqual,
	notStrictEqual:		notStrictEqual,
	ok:					ok,
	strictEqual:		strictEqual,
	throws:             throws,
	AssertionError:		AssertionError,
};

const assert = require('assert');

function deepEqual(actual, expected, message)
{
	if (_equiv(actual, expected, false))
		return;
	fail(actual, expected, message, 'deepEqual');
}

function deepStrictEqual(actual, expected, message)
{
	if (_equiv(actual, expected, true))
		return;
	fail(actual, expected, message, 'strictDeepEqual');
}

function equal(actual, expected, message)
{
	if (actual == expected)
		return;
	fail(actual, expected, message, '==');
}

function fail(actual, expected, message, operator)
{
	throw new AssertionError({
		message:  message,
		actual:	  actual,
		expected: expected,
		operator: operator,
	});
}

function notDeepEqual(actual, expected, message)
{
	if (!_equiv(actual, expected, false))
		return;
	fail(actual, expected, message, 'notDeepEqual');
}

function notDeepStrictEqual(actual, expected, message)
{
	if (!_equiv(actual, expected, true))
		return;
	fail(actual, expected, message, 'notDeepStrictEqual');
}

function notEqual(actual, expected, message)
{
	if (actual != expected)
		return;
	fail(actual, expected, message, '!=');
}

function notStrictEqual(actual, expected, message)
{
	if (actual !== expected)
		return;
	fail(actual, expected, message, '!==');
}

function ok(guard, message)
{
	equal(guard, true, message);
}

function strictEqual(actual, expected, message)
{
	if (actual === expected)
		return;
	fail(actual, expected, message, '===');
}

function throws(block, expected, message)
{
	if (typeof block !== 'function')
		throw new TypeError("`block` argument is not callable");
	if (typeof expected === 'string') {
		message = expected;
		expected = undefined;
	}

	var hasThrown = false;
	var actual;
	try {
		block();
	}
	catch (err) {
		hasThrown = true;
		actual = err;
	}
	if (hasThrown) {
		if (expected instanceof RegExp && expected.test(actual))
			return;
		else if (typeof expected === 'function') {
			if (expected === Error || expected.prototype instanceof Error) {
				if (actual instanceof expected)
					return;
				throw actual;
			}
			if (expected(actual))
				return;
			throw actual;
		}
		else
			throw actual;
	}
	fail(block, expected, message, "throws");
}

AssertionError.prototype = Object.create(Error.prototype);
AssertionError.prototype.constructor = AssertionError;
AssertionError.prototype.name = "AssertionError";
function AssertionError(options)
{
	this.actual = options.actual;
	this.expected = options.expected;
	this.operator = options.operator;
	this.message = options.message
		|| this.actual + " " + this.operator + " " + this.expected;
}

function _equiv(a, b, strict)
{
	// equality implies equivalence, so we avoid a ton of unnecessary work by
	// testing for that first.	as a nice side effect, if we don't return here,
	// the rest of the function can safely assume the values are non-equal,
	// which avoids a lot of extra comparisons.
	if (a === b || (!strict && a == b))
		return true;

	if (typeof a !== 'object' || typeof b !== 'object' || a === null || b === null)
		return false;  // non-equal primitives are never equivalent

	// if we reach this point, it means both values are object references, so
	// we perform a recursive memberwise comparison to test equivalence.
	var aPrototype = Object.getPrototypeOf(a);
	var bPrototype = Object.getPrototypeOf(b);
	if (strict && aPrototype !== bPrototype)
		return false;
	var aKeys = Object.keys(a).sort();
	var bKeys = Object.keys(b).sort();
	if (aKeys.length !== bKeys.length)
		return false;
	for (var i = aKeys.length - 1; i >= 0; --i) {
		if (aKeys[i] !== bKeys[i])
			return false;
		var valueA = a[aKeys[i]];
		var valueB = b[bKeys[i]];
		if (!_equiv(valueA, valueB, strict))
			return false;
	}
	return true;
}
