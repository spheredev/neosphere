minisphere 3.0
==============

[![Build Status](https://travis-ci.org/fatcerberus/minisphere.svg?branch=master)]
(https://travis-ci.org/fatcerberus/minisphere)

minisphere is a drop-in replacement and successor to the Sphere game engine,
written from the ground up in C.  It boasts a high level of compatibility with
most games written for Sphere 1.x, with better performance and new functionality.
The majority of games will run with no modifications.

Like Sphere, minisphere uses JavaScript for game coding.  The engine exposes a
collection of low-level functions through a standardized JavaScript API, leaving
higher-level game logic entirely up to scripts. This allows any type of game to
be made with minisphere, but of course requires more expertise than making a
game with, say, RPG Maker or even Game Maker.

The engine uses Allegro 5 for graphics and sound and Duktape for JavaScript. As
both of these are portable to various platforms, this allows minisphere to be
compiled successfully on all three major platforms (Windows, Linux, and OS X)--
and possibly others--with no changes to the source.

minisphere is not just a game engine, but a complete toolchain for game
development. Several command-line tools, described below, are included to make
game development easier:

Fully Automated Build System
----------------------------

minisphere comes with a flexible script-driven build system called Cell, which
can not only package Sphere games into easy-to-distribute SPKs, but also allows
generating assets from easier-to-modify sources.  For example: Building tilesets
from images, or minifying scripts at build time.

Cell's design borrows heavily from SCons, with a declarative approach to
scripting. This may be confusing for those attempting to jump right in; it is
highly recommend to read the manual page for Cell before trying to write a
Cellscript for your game.

Powerful Console Debugger
-------------------------

minisphere includes a powerful but easy-to-use command-line debugger, called
SSJ. The debugger allows you to step through your game's code and inspect the
internal state of the game--variables, call stack, objects, etc.--as it
executes.  And since minisphere uses JavaScript, the original source files
aren't required to be present--SSJ can download source code directly from the
minisphere instance being debugged.

A symbolic debugger such as SSJ is an invaluable tool for development and is a
minisphere exclusive: No similar tool was ever available for Sphere 1.x.


License
=======

minisphere and accompanying command-line tools are licensed under the terms of
the BSD-3-clause license. Practically speaking, this means the engine can be
used for any purpose, even commercially, with no restriction other than maintain
the original copyright notice.


Download
========

Official binaries for minisphere are released through GitHub and the latest
stable version can always be downloaded directly from the GitHub releases page
here:

<https://github.com/fatcerberus/minisphere/releases>

The current minisphere release at the time of this writing is
[minisphere 3.0b4](https://github.com/fatcerberus/minisphere/releases/tag/v3.0b4).
Note that this is a pre-release, and while it is well-tested in-house, it may
exhibit minor bugs. It should be quite usable for everyday game development,
however.
