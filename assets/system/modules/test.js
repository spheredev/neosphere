/**
 *  miniRT test CommonJS module
 *  (c) 2015-2016 Fat Cerberus
**/

'use strict';
module.exports =
{
	run: run
};

const from = require('from');

function run(tests)
{
	from.Object(tests)
		.where(function(test, k) { return k.substring(0, 4) === 'test'; })
		.where(function(test, k) { return k !== 'test' })
		.each(function(test)
	{
		if (typeof test === 'function')
			test();
		else if (typeof test === 'object' && test !== null)
			run(test);
	});
}
