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

const Corpus = "1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

var haveY = false;
var y;

function chance(odds)
{
    system.assert(typeof odds === 'number', "odds must be a number");

    return system.random() < odds;
}

function discrete(min, max)
{
    system.assert(typeof min === 'number', "min must be a number");
    system.assert(typeof max === 'number', "max must be a number");

    min >>= 0;
    max >>= 0;
    var range = Math.abs(max - min) + 1;
    min = min < max ? min : max;
    return Math.floor(system.random() * range);
}

function normal(mean, sigma)
{
    system.assert(typeof mean === 'number', "mean must be a number");
    system.assert(typeof sigma === 'number', "sigma must be a number");

    if (!haveY) {
        do {
            var u = 2.0 * system.random() - 1.0;
            var v = 2.0 * system.random() - 1.0;
            var w = u * u + v * v;
        } while (w >= 1.0);
        w = Math.sqrt(-2.0 * Math.log(w) / w);
        var x = u * w;
        y = v * w;
        haveY = true;
    }
    else {
        x = y;
        haveY = false;
    }
    return mean + x * sigma;
}

function sample(array)
{
    system.assert(Array.isArray(array), "argument must be an array");

    var index = discrete(0, array.length);
    return array[index];
}

function string(length)
{
    if (length === void null)
        length = 10;

    system.assert(typeof length === 'number', "length must be a number");
    system.assert(length > 0, "length must be greater than zero");

    length >>= 0;
    var string = "";
    for (var i = 0; i < length; ++i) {
        var index = discrete(0, Corpus.length);
        string += Corpus[index];
    }
    return string;
}

function uniform(mean, variance)
{
    system.assert(typeof mean === 'number', "mean must be a number");
    system.assert(typeof variance === 'number', "variance must be a number");
    
    var error = variance * 2.0 * (0.5 - system.random());
    return mean + error;
}
