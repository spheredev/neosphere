neoSphere: a JavaScript-based game engine
=========================================

[![Release](https://img.shields.io/github/release/fatcerberus/neosphere.svg)](https://github.com/fatcerberus/sphere/releases/latest)

**neoSphere** is a lightweight, fully sandboxed, JavaScript-powered game
engine and development platform.  It is a complete rewrite of Chad Austin's
original *Sphere* engine, written from the ground up in C.  While neoSphere is
backwards compatible with the original engine (henceforth referred to as
**Sphere 1.x**), it also brings with it a new, modern API and a collection of
powerful development tools.

Graphics and audio support are provided through
[Allegro 5](http://liballeg.org), and JavaScript support is enabled by
[ChakraCore](https://github.com/Microsoft/ChakraCore), the same JavaScript
engine that once powered Microsoft Edge.


Download
========

**neoSphere 5.7.2**, released on January 25, 2022, can be downloaded from the
neoSphere GitHub Releases page:

* <https://github.com/fatcerberus/neosphere/releases>

For an overview of the breaking changes in each release, refer to
[`RELEASES.md`](RELEASES.md).


The Sphere Toolbox
==================

Sphere games are written in JavaScript, which gives game developers a great
deal of power.  The API is designed to be user-friendly and reasonably
accessible to novices, allowing coders of any skill level to pick up the engine
and start hacking away.

The Game Engine
---------------

The official Sphere engine implementation for desktop platforms is known as
**neoSphere**.  There is also a Web-based version, currently in development,
known as **Oozaru**, which has its own GitHub repository but is planned to
eventually be merged into the main repository. 

Sphere games are written in JavaScript.  The engine exposes a collection of
low-level functions to the JavaScript environment, leaving higher-level game
logic entirely up to script.  In this way, any kind of game can be developed
and you are not limited to a specific genre as in, for example, RPG Maker.

Besides the low-level Core API, there is also a set of libraries written in
JavaScript that any game is free to use, collectively called the
Sphere Runtime.  The Runtime provides high-level functionality such as a
text-based debug console and music manager, which can save a lot of time when
developing new games.


The Compiler
------------

**Cell**, the Sphere compiler, uses JavaScript to control the build process.
Like neoSphere, Cell is powered by ChakraCore and natively supports ES2015
features like arrow functions, destructuring, and modules.  A basic Cellscript
might look like this:

```js
/* Cellscript.js */

describe({
    version:  2,  // target the Sphere v2 API
    apiLevel: 3,  // require API level 3+

    name:       "My Game",
    author:     "Some Guy",
    summary:    "This game is awesome.",
    resolution: '768x480',
    main:       'scripts/main.js',
});

// install JavaScript modules
install('@/scripts', files('scripts/*.js', true));

// install non-code game assets
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
Sphere games.  SSj works in tandem with neoSphere to allow you to step through
your game's code and inspect the internal state of the code--variables, call
stack, objects, etc.--while it executes.  Best of all, the original source
files don't need to be present: SSj can download source code directly from the
engine instance being debugged.

A symbolic debugger like SSj is an invaluable tool for development and is a
neoSphere exclusive: no such tool was ever available for the original engine!


License
=======

The entire Sphere game development platform is licensed under the terms of the
BSD-3-clause license.  Practically speaking, this means the engine and tools
can be used for any purpose, even commercially, with no restriction other than
maintain the original copyright notice.
