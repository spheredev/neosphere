/**
 *  Sphere Runtime for Cellscripts
 *  Copyright (c) 2015-2017, Fat Cerberus
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

'use strict';
exports.__esModule = true;
exports.default = exports;

const assert = require('assert');

var rng = new RNG();

exports.chance = chance;
function chance(odds)
{
	assert.ok(typeof odds === 'number', "odds must be a number");

	return odds > rng.next();
}

exports.discrete = discrete;
function discrete(min, max)
{
	assert.ok(typeof min === 'number', "min must be a number");
	assert.ok(typeof max === 'number', "max must be a number");

	min >>= 0;
	max >>= 0;
	var range = Math.abs(max - min) + 1;
	min = min < max ? min : max;
	return min + Math.floor(rng.next() * range);
}

exports.normal = normal;
function normal(mean, sigma)
{
	assert.ok(typeof mean === 'number', "mean must be a number");
	assert.ok(typeof sigma === 'number', "sigma must be a number");

	// normal deviates are calculated in pairs.  we return the first one
	// immediately, and save the second to be returned on the next call to
	// random.normal().
	var x, u, v, w;
	if (normal.memo === undefined) {
		do {
			u = 2.0 * rng.next() - 1.0;
			v = 2.0 * rng.next() - 1.0;
			w = u * u + v * v;
		} while (w >= 1.0);
		w = Math.sqrt(-2.0 * Math.log(w) / w);
		x = u * w;
		normal.memo = v * w;
	}
	else {
		x = normal.memo;
		delete normal.memo;
	}
	return mean + x * sigma;
}

exports.sample = sample;
function sample(array)
{
	assert.ok(Array.isArray(array), "argument must be an array");

	var index = discrete(0, array.length - 1);
	return array[index];
}

exports.string = string;
function string(length)
{
	const CORPUS = "1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

	length = length !== undefined ? length : 10;

	assert.ok(typeof length === 'number', "length must be a number");
	assert.ok(length > 0, "length must be greater than zero");

	length >>= 0;
	var string = "";
	for (var i = 0; i < length; ++i) {
		var index = discrete(0, CORPUS.length - 1);
		string += CORPUS[index];
	}
	return string;
}

exports.uniform = uniform;
function uniform(mean, variance)
{
	assert.ok(typeof mean === 'number', "mean must be a number");
	assert.ok(typeof variance === 'number', "variance must be a number");

	var error = variance * 2.0 * (0.5 - rng.next());
	return mean + error;
}
