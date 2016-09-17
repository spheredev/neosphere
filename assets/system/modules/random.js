/**
 *  miniRT random CommonJS module
 *  (c) 2016 Fat Cerberus
**/

'use strict';
module.exports =
{
    chance:   chance,
    discrete: discrete,
    normal:   normal,
    sample:   sample,
    string:   string,
    uniform:  uniform,
};

const CORPUS = "1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

var rng = new RNG();

function chance(odds)
{
    SSJ.assert(typeof odds === 'number', "odds must be a number");

    return odds > rng.next();
}

function discrete(min, max)
{
    SSJ.assert(typeof min === 'number', "min must be a number");
    SSJ.assert(typeof max === 'number', "max must be a number");

    min >>= 0;
    max >>= 0;
    var range = Math.abs(max - min) + 1;
    min = min < max ? min : max;
    return min + Math.floor(rng.next() * range);
}

normal.y = null;
function normal(mean, sigma)
{
    SSJ.assert(typeof mean === 'number', "mean must be a number");
    SSJ.assert(typeof sigma === 'number', "sigma must be a number");

    // normal deviates are calculated in pairs.  we return the first one
    // immediately, and save the second to be returned on the next call to
    // random.normal().
    if (normal.y === null) {
        do {
            var u = 2.0 * rng.next() - 1.0;
            var v = 2.0 * rng.next() - 1.0;
            var w = u * u + v * v;
        } while (w >= 1.0);
        w = Math.sqrt(-2.0 * Math.log(w) / w);
        var x = u * w;
        normal.y = v * w;
    }
    else {
        x = normal.y;
        normal.y = null;
    }
    return mean + x * sigma;
}

function sample(array)
{
    SSJ.assert(Array.isArray(array), "argument must be an array");

    var index = discrete(0, array.length - 1);
    return array[index];
}

function string(length)
{
    if (length === void null)
        length = 10;

    SSJ.assert(typeof length === 'number', "length must be a number");
    SSJ.assert(length > 0, "length must be greater than zero");

    length >>= 0;
    var string = "";
    for (var i = 0; i < length; ++i) {
        var index = discrete(0, CORPUS.length - 1);
        string += CORPUS[index];
    }
    return string;
}

function uniform(mean, variance)
{
    SSJ.assert(typeof mean === 'number', "mean must be a number");
    SSJ.assert(typeof variance === 'number', "variance must be a number");
    
    var error = variance * 2.0 * (0.5 - rng.next());
    return mean + error;
}
