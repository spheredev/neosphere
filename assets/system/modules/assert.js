/**
 *  miniRT assert CommonJS module
 *  (c) 2015-2016 Fat Cerberus
**/

'use strict';
module.exports = assert;

assert.ok = assert;
function assert(guard, message)
{
	equal(!!guard, true, message);
}

assert.deepEqual = deepEqual;
function deepEqual(actual, expected, message)
{
	if (_equiv(actual, expected, false))
		return;
	fail(actual, expected, message, 'deepEqual');
}

assert.deepStrictEqual = deepStrictEqual;
function deepStrictEqual(actual, expected, message)
{
	if (_equiv(actual, expected, true))
		return;
	fail(actual, expected, message, 'strictDeepEqual');
}

assert.doesNotThrow = doesNotThrow;
function doesNotThrow(block, expected, message)
{
	if (typeof block !== 'function')
		throw new TypeError("`block` argument not callable");
	if (typeof expected === 'string') {
		message = expected;
		expected = undefined;
	}

	if (!_checkIfThrows(block, expected))
		return;
	fail(block, expected, message || "unwanted exception");
}

assert.equal = equal;
function equal(actual, expected, message)
{
	if (actual == expected)
		return;
	fail(actual, expected, message, '==');
}

assert.fail = fail;
function fail(actual, expected, message, operator)
{
	throw new AssertionError({
		message:  message,
		actual:	  actual,
		expected: expected,
		operator: operator,
	});
}

assert.notDeepEqual = notDeepEqual;
function notDeepEqual(actual, expected, message)
{
	if (!_equiv(actual, expected, false))
		return;
	fail(actual, expected, message, 'notDeepEqual');
}

assert.notDeepStrictEqual = notDeepStrictEqual;
function notDeepStrictEqual(actual, expected, message)
{
	if (!_equiv(actual, expected, true))
		return;
	fail(actual, expected, message, 'notDeepStrictEqual');
}

assert.notEqual = notEqual;
function notEqual(actual, expected, message)
{
	if (actual != expected)
		return;
	fail(actual, expected, message, '!=');
}

assert.notStrictEqual = notStrictEqual;
function notStrictEqual(actual, expected, message)
{
	if (actual !== expected)
		return;
	fail(actual, expected, message, '!==');
}

assert.strictEqual = strictEqual;
function strictEqual(actual, expected, message)
{
	if (actual === expected)
		return;
	fail(actual, expected, message, '===');
}

assert.throws = throws;
function throws(block, expected, message)
{
	if (typeof block !== 'function')
		throw new TypeError("`block` argument is not callable");
	if (typeof expected === 'string') {
		message = expected;
		expected = undefined;
	}

	if (_checkIfThrows(block, expected))
		return;
	fail(block, expected, message || "no exception thrown");
}

assert.AssertionError = AssertionError;
AssertionError.prototype = Object.create(Error.prototype);
AssertionError.prototype.constructor = AssertionError;
AssertionError.prototype.name = "AssertionError";
Object.setPrototypeOf(AssertionError, Error);
function AssertionError(options)
{
	if (!(this instanceof AssertionError))
		return new AssertionError(options);
	this.actual = options.actual;
	this.expected = options.expected;
	this.message = options.message
		|| this.actual + " " + options.operator + " " + this.expected;
}

function _checkIfThrows(block, expected)
{
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
			return true;
		else if (typeof expected === 'function') {
			if (expected === Error || expected.prototype instanceof Error) {
				if (actual instanceof expected)
					return true;
				throw actual;
			}
			if (expected(actual))
				return true;
			throw actual;
		}
		else
			throw actual;
	}
	return false;
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
