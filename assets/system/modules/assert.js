/**
 *  miniRT assert CommonJS module
 *  (c) 2015-2016 Fat Cerberus
**/

'use strict';
module.exports =
{
    equal:          equal,
    fail:           fail,
    notEqual:       notEqual,
    notStrictEqual: notStrictEqual,
    ok:             ok,
    strictEqual:    strictEqual,
    AssertionError: AssertionError,
};

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
        actual:   actual,
        expected: expected,
        operator: operator,
    });
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
