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
function assert(guard, message)
{
	equal(Boolean(guard), true, message);
}

assert.ok = assert;

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
	let hasThrown = false;
	let actual;
	try {
		block();
	}
	catch (err) {
		hasThrown = true;
		actual = err;
	}
	if (hasThrown) {
		if (expected instanceof RegExp && expected.test(actual)) {
			return true;
		}
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
		else {
			throw actual;
		}
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
	let aPrototype = Object.getPrototypeOf(a);
	let bPrototype = Object.getPrototypeOf(b);
	if (strict && aPrototype !== bPrototype)
		return false;
	let aKeys = Object.keys(a).sort();
	let bKeys = Object.keys(b).sort();
	if (aKeys.length !== bKeys.length)
		return false;
	for (let i = aKeys.length - 1; i >= 0; --i) {
		if (aKeys[i] !== bKeys[i])
			return false;
		let valueA = a[aKeys[i]];
		let valueB = b[bKeys[i]];
		if (!_equiv(valueA, valueB, strict))
			return false;
	}
	return true;
}
