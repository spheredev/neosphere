/**
 * minisphere Runtime 1.1 - (c) 2015 Fat Cerberus
 * A set of system scripts providing advanced, high-level functionality not
 * available in the engine itself.
 *
 * [mini/RNG.js]
 * A polyfill for minisphere's built-in RNG object which can be used
 * in any JavaScript environment.
**/

// note: This uses JavaScript's Math.random(), which cannot be manually seeded.
// As a result, RNG.seed() is included, but is a no-op with this implementation.

RequireSystemScript('mini/Core.js');

if (typeof RNG === 'undefined')
{
	// RNG object
	// Encapsulates the random number generator.
	RNG = new (function()
	{
		this.nextND = null;
	})();
	
	// RNG.seed()
	// Seeds the random number generator. As there is no way to supply a seed
	// for Math.random(), this method actually does nothing.
	// Arguments:
	//     seed: The seed value.
	RNG.seed = function(seed)
	{
		// no-op
	};
	
	// RNG.chance()
	// Tests a percentage chance.
	// Arguments:
	//     odds: The odds of the chance passing, specified as a decimal from 0 to 1.
	//           Chances <= 0 will never pass, and >= 1 will always pass.
	RNG.chance = function(odds)
	{
		return odds > Math.random();
	};
	
	// RNG.normal()
	// Returns a random value from a normal probability distribution, sometimes
	// called a "bell curve."
	// Arguments:
	//     mean:  The mean value of the distribution.
	//     sigma: The standard deviation.
	RNG.normal = function(mean, sigma)
	{
		if (this.nextND === null) {
			var u, v, w;
			do {
				u = 2.0 * Math.random() - 1.0;
				v = 2.0 * Math.random() - 1.0;
				w = u * u + v * v;
			} while (w >= 1.0);
			w = Math.sqrt(-2 * Math.log(w) / w);
			var x = u * w;
			this.nextND = v * w;
			return mean + sigma * x;
		} else {
			var y = this.nextND;
			this.nextND = null;
			return mean + sigma * y;
		}
	};
	
	// RNG.range()
	// Returns a random integer uniformly distributed within a specified range.
	// Arguments:
	//     min: The minimum value.
	//     max: The maximum value.
	RNG.range = function(min, max)
	{
		var size = max - min + 1;
		return min + Math.min(Math.floor(Math.random() * size), size - 1);
	};
	
	// RNG.random()
	// The basic generation method; returns a value in the range [0,1).
	RNG.random = function()
	{
		return Math.random();
	}
	
	// RNG.sample()
	// Returns a random item selected from an array.
	// Arguments:
	//     array: An array of items to choose from.
	RNG.sample = function(array)
	{
		var index = Math.min(Math.floor(Math.random() * array.length), array.length - 1);
		return array[index];
	};
	
	// RNG.vary()
	// Applies uniform random variance to a specified value.
	// Arguments:
	//     mean:     The mean value around which to apply variance.
	//     variance: The maximum amount by which the output can deviate from the mean,
	//               in either direction.
	// Returns:
	//     The new value after random variance has been applied.
	RNG.vary = function(mean, variance)
	{
		var error = variance * 2 * (0.5 - Math.random());
		return value + error;
	};
}
