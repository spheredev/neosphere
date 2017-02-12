miniSphere
==========

[![Build Status](https://travis-ci.org/fatcerberus/minisphere.svg?branch=master)]
(https://travis-ci.org/fatcerberus/minisphere)

miniSphere is the successor to Chad Austin's original *Sphere* JavaScript game
engine, written from the ground up in C.  Games are written entirely in
JavaScript, providing a powerful development platform on par with writing a
game in C++.  The engine can be compiled on all three major platforms (Windows,
macOS, and Linux) with no changes to its source code.

Sphere games are written in JavaScript.  The engine exposes a collection of
low-level functions through the JavaScript environment, leaving higher-level
game logic entirely up to script.  This allows any type of game you can imagine
to be made; of course, this naturally requires more expertise than making a
game in, say, RPG Maker or Game Maker, but the ultimate flexibility you get in
return is very often worth it.

The engine uses [Allegro 5](https://github.com/liballeg/allegro5) on the
backend and the [Duktape](http://duktape.org/) JavaScript engine on the
frontend.  As both of these are portable to various platforms, miniSphere can
be compiled successfully on all three major platforms (Windows, Linux, and
OS X)--and possibly others--with no changes to the source.

Cell: the Sphere compiler
-------------------------

Like miniSphere itself, its compiler, **Cell**, also uses JavaScript to control
the build process.  Out of the box, Cell supports ECMAScript 2015
transpilation, allowing you to use new JavaScript syntax such as classes and
arrow functions in your game code.  A basic Cellscript might look like this:

```js
import transpile from 'transpile';
 
// whatever is passed to describe() is written to game.json.
describe("Game Title",
{
    author: "Somebody",
    summary: "This game is probably awesome.",
    resolution: '320x240',
    main: 'scripts/main.js',  // main JS module
});
 
// miniSphere doesn't support ES 2015 syntax natively.  transpile() builds
// compatible scripts from ES 2015 code.
transpile('@/scripts', files('scripts/*.js', true));
 
// install assets into the game package
install('@/images', files('images/*.png', true));
install('@/music',  files('music/*.ogg', true));
install('@/sounds', files('sounds/*.wav', true));
install('@/',       files('icon.png'));
```

Using Cell's flexible Tool API, the compiler can be extended to perform any
kind of build.  For example, the Tool below will copy a single source file:

```js
// Second argument is optional and specifies how the engine describes the
// tool's job.  If omitted, Cell just says "building" when running the tool.
const CopyTool = new Tool((outName, sourceNames) => {
    let buffer = FS.readFile(sourceNames[0]);
    FS.writeFile(outName, buffer);
}, "copying");

// Invoke the Tool like so (note: destination filename comes first):
CopyTool.stage('@/eatypig.fat', [ 'eaty/pig.src' ]);
```

SSJ: powerful JavaScript debugging
----------------------------------

miniSphere includes an easy-to-use command-line debugger, called SSJ.  SSJ
allows you to step through your game's code and inspect the internal state of
the game--variables, call stack, objects, etc.--as it executes.  Best of all,
the original source files don't need to be present, as SSJ can download source
code directly from the miniSphere instance being debugged.

A symbolic debugger like SSJ is an invaluable tool for development and is a
miniSphere exclusive: No similar tool was ever made available for the original
Sphere engine.


Download
========

The latest stable miniSphere release at the time of this writing is
**miniSphere 4.4.4**, released on Monday, February 6, 2017.  miniSphere
binaries are provided through GitHub, and the latest version is always
available for download here:

* <https://github.com/fatcerberus/minisphere/releases>

For an overview of breaking changes in the current major stable release series,
refer to [`RELEASES.md`](RELEASES.md).


License
=======

miniSphere and its accompanying command-line tools are licensed under the terms
of the BSD-3-clause license.  Practically speaking, this means the engine can
be used for any purpose, even commercially, with no restriction other than
maintain the original copyright notice.
