/**
 *  miniRT 2.0 aggregate CommonJS module
 *  provides common functionality for Sphere 2.0 games
 *  (c) 2015-2016 Fat Cerberus
**/

if (typeof exports === 'undefined')
{
	throw new TypeError("miniRT must be loaded using require()");
}

module.exports = (function()
{
	var threads   = require('./miniRT/threads');
	var console   = require('./miniRT/console');
	var delegates = require('./miniRT/delegates');
	var music     = require('./miniRT/music');
	var pacts     = require('./miniRT/pacts');
	var scenes    = require('./miniRT/scenes');

	return {
		threads:  threads,
		console:  console,
		music:    music,
		scenelet: scenes.scenelet,
		Delegate: delegates.Delegate,
		Pact:     pacts.Pact,
		Promise:  pacts.Promise,
		Scene:    scenes.Scene,
	};
})();
