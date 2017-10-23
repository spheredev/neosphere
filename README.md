miniSphere
==========

[![Build Status](https://travis-ci.org/fatcerberus/minisphere.svg?branch=master)](https://travis-ci.org/fatcerberus/minisphere)

**miniSphere** is a lightweight JavaScript-powered game engine and the official
successor to Chad Austin's original *Sphere* game engine, written from the
ground up in C.  [Allegro 5](http://liballeg.org) is used on the backend, and
JavaScript support is provided by [ChakraCore](https://github.com/Microsoft/ChakraCore),
the same JavaScript engine that powers Microsoft Edge.

Sphere games are written in JavaScript, which gives game developers a great
deal of power.  The API is also designed to be user-friendly and reasonably
accessible to novices, allowing coders of any skill level to pick up the engine
and start hacking away.

The game engine and associated command line tools are cross-platform and fairly
portable.  miniSphere is officially supported on both Windows and Linux, and
has been successfully built on macOS.  Mobile support (both iOS and Android) is
currently being considered, although there are no concrete plans on that front
just yet.


Download
========

The latest stable miniSphere release at the time of this writing is
**miniSphere 4.8.8**, released on Thursday, September 21, 2017.  miniSphere
binaries are provided through GitHub, and the latest version is always
available for download here:

* <https://github.com/fatcerberus/minisphere/releases>

For an overview of breaking changes in the current stable release series, refer
to [`RELEASES.md`](RELEASES.md).


The Sphere Toolchain
====================

The Game Engine
---------------

The official Sphere game engine is known as **miniSphere**.

Sphere games are written in JavaScript.  The engine exposes a collection of
low-level functions to the JavaScript environment, leaving higher-level game
logic entirely up to script.  In this way, any kind of game you can imagine can
be developed.

Besides the low-level Core API, there is also a set of libraries written in
JavaScript that any game is free to use, collectively called the
Sphere Runtime.  The Runtime provides high-level functionality such as a
text-based debug console and music manager, which can save a lot of time when
developing new games.


The Compiler
------------

**Cell**, the Sphere compiler, uses JavaScript to control the build process.
Like miniSphere, Cell is powered by ChakraCore and natively supports ES2015
features like arrow functions, destructuring, and modules.  A basic Cellscript
might look like this:

```js
/* Cellscript.mjs */

// all values defined on Sphere.Game are JSON encoded at the end of the build
// and written to game.json.
Object.assign(Sphere.Game,
{
    name:       "My Game",
    author:     "Some Guy",
    summary:    "This game is awesome.",
    resolution: '320x240',
    main:       '@/scripts/main.mjs',
});

// install modules and scripts.  starting with miniSphere 5.0, Sphere supports
// ES2015+ out of the box so no transpile is necessary!
install('@/scripts', files('src/*.mjs', true));
install('@/scripts', files('src/*.js', true));

// install game assets
install('@/images', files('images/*.png', true));
install('@/music',  files('music/*.ogg', true));
install('@/sounds', files('sounds/*.wav', true));
install('@/',       files('icon.png'));
```

If that's not enough for you, then using Cell's flexible `Tool` API, you can
extend the compiler to build any kind of asset.  Simply construct a `Tool`
object and provide a function to be called when the tool is executed.  The tool
below will write the contents of a single text file to the destination file:

```js
// the second argument to `new Tool()` is optional and describes the process
// performed by the tool, e.g., "compiling" or "installing".  if omitted, Cell
// just says "building".
let copyTool = new Tool((outName, sourceNames) => {
    let text = FS.readFile(sourceNames[0]);
    FS.writeFile(outName, text);
}, "copying");
```

To have Cell build a file using your new tool, just do this:

```js
// the first argument is the destination filename, the second argument is an
// array of Target objects specifying the sources.  Here we pass an exact
// filename to files() to get that array.
//
// note: the prefix '@/' refers to the root of the game package being compiled.
//       see `cellscript-api.txt` for more information on SphereFS prefixes.
copyTool.stage('@/eatypig.fat', files('eaty/pig.src'));
```


The Debugger
------------

**SSj**, the Sphere debugger, is an easy-to-use command-line debugger for
Sphere games.  The debugger allows you to step through your game's code and
inspect the internal state of the code--variables, call stack, objects,
etc.--while it executes.  Best of all, the original source files don't need to
be present: SSj can download source code directly from the engine instance
being debugged.

A symbolic debugger like SSj is an invaluable tool for development and is a
miniSphere exclusive: no such tool was ever available for the original engine!


License
=======

miniSphere and its accompanying command-line tools are licensed under the terms
of the BSD-3-clause license.  Practically speaking, this means the engine can
be used for any purpose, even commercially, with no restriction other than
maintain the original copyright notice.
