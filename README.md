miniSphere
==========

[![Build Status](https://travis-ci.org/fatcerberus/minisphere.svg?branch=master)]
(https://travis-ci.org/fatcerberus/minisphere)

miniSphere is the successor to Chad Austin's original *Sphere* JavaScript game
engine, written from the ground up in C.  Games are written entirely in
JavaScript, providing a powerful development platform on par with writing a 2D
game in C++.  The engine can be compiled on all three major platforms (Windows,
macOS, and Linux) with no changes to its source code.

Overview
--------

Like Sphere before it, miniSphere games are written in JavaScript.  The game
engine exposes a collection of low-level functions through a standardized
JavaScript API, leaving higher-level game logic entirely up to script.  This
allows any type of game to be developed; of course, it naturally requires more
expertise than making a game with, say, RPG Maker or even Game Maker, but the
ultimate flexibility you get in return is often worth it.

The engine uses Allegro 5 on the backend and the [Duktape](http://duktape.org/)
JavaScript engine on the frontend.  As both of these are portable to various
platforms, this allows miniSphere to be compiled successfully on all three
major platforms (Windows, Linux, and OS X)--and possibly others--with no
changes to the source.

Cell: the Sphere v2 compiler
----------------------------

Like miniSphere itself, its compiler, **Cell**, also uses JavaScript to control
the build process.  Out of the box, Cell supports ECMAScript 2015
transpilation, allowing you to use new JavaScript syntax such as classes and
arrow functions in your game code.


SSJ: powerful JavaScript debugging
----------------------------------

miniSphere includes a fairly powerful but easy-to-use command-line debugger,
called SSJ.  SSJ allows you to step through your game's code and inspect the
internal state of the game--variables, call stack, objects, etc.--as it
executes.  Best of all, the original source files don't need to be present--
SSJ can download source code directly from the miniSphere instance being
debugged.

A symbolic debugger such as SSJ is an invaluable tool for development and is a
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
