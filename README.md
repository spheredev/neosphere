minisphere 3.0
==============

[![Build Status](https://travis-ci.org/fatcerberus/minisphere.svg?branch=master)]
(https://travis-ci.org/fatcerberus/minisphere)

minisphere is a drop-in replacement and successor to the Sphere game engine,
written from the ground up in C.  It boasts a high level of compatibility with
most games written for Sphere 1.x, with better performance and new functionality.
The majority of games will run with no modifications.

Like Sphere, minisphere uses JavaScript for game coding. The engine exposes a
collection of low-level functions through a standardized JavaScript API, leaving
higher-level game logic entirely up to scripts. This allows any type of game to
be made with minisphere, but of course requires more expertise than making a
game with, say, RPG Maker or even Game Maker.

The engine uses Allegro 5 for graphics and sound and Duktape for JavaScript. As
both of these are portable to various platforms, this allows minisphere to be
compiled successfully on all three major platforms (Windows, Linux, and OS X)--
and possibly others--with no changes to the source.

Cell
----

minisphere comes with a powerful script-driven build system called Cell, which
can not only build a Sphere game package, but also allows generating assets from
easier-to-modify sources--for example, building tilesets from images or
minifying scripts at build time.

SSJ
------------------

In version 3.0 and later, minisphere comes with a GDB-inspired console debugger
called SSJ. SSJ allows single-stepping through a game's JavaScript code and
in inspecting the internal state (variables, call stack, objects, etc.) of a
game as it executes. This is a very useful tool for diagnosing bugs and is
exclusive to minisphere--no similar program has ever been available for
Sphere 1.x.


License
=======

minisphere is licensed under the terms of the BSD 3-clause license. Practically
speaking, this means the engine can be used for any purpose, even commercially,
with no restriction other than maintain the original copyright notice.


System Requirements
===================

minisphere requires somewhat modern hardware to run adequately.  The official
Windows release requires a 64-bit processor and operating system; starting with
minisphere 3.0, there is no officially-supported 32-bit Windows build.  A decent
graphics card is also required, supporting at least OpenGL 2.0 for full
functionality.


Command Line Usage
==================

```
spherun [options] [file_or_dir]

file_or_dir can be either a directory, or the name of a file to open, either
an SPK package or a bare JS script to execute.

--fullscreen        Start the engine in full-screen mode.
--window            Start the engine in windowed mode.
--frameskip <x>     Set the initial frameskip limit to <x>. Note that if the
					game calls SetMaxFrameSkips(), that value overrides this
                    one.  The default frameskip limit is 5.
--no-sleep          Prevent the engine from sleeping between frames. This may
                    improve performance on slower machines at the cost of maxing
					out a processor core.
--log-level <0-4>   Set the console verbosity. 0 is practically silent, while 4
                    is quite noisy and useful for diagnostics. The default
                    setting is 1.
```
