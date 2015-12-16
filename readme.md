minisphere 2.1
==============

[![Build Status](https://travis-ci.org/fatcerberus/minisphere.svg?branch=master)]
(https://travis-ci.org/fatcerberus/minisphere)

minisphere is a drop-in replacement and successor to the Sphere game engine,
written from the ground up in C.  It boasts a high level of compatibility with
most games written for Sphere 1.x, with better performance and new functionality.
The majority of games will run with no modifications.

minisphere is small, living up to its name: The engine in its entirety consists
of a single executable just over 2 MB in size, plus around 200KB or so for the
system assets. This makes it very easy to distribute.

Like Sphere, minisphere uses JavaScript for game coding. The engine exposes a
collection of low-level functions through a standardized JavaScript API, leaving
higher-level game logic entirely up to scripts. This allows any type of game to be
made with minisphere, but of course requires more expertise than making a game
with, say, RPG Maker or even Game Maker.

The engine uses Allegro 5.1 for graphics and sound and Duktape for JavaScript.
As both of these are highly portable to various platforms, this allows
minisphere to be compiled successfully on all three major platforms (Windows,
Linux, and OS X) with no changes to the source.

License
-------

minisphere is licensed under the terms of the 3-clause BSD license. Practically
speaking, this means the engine can be used for any purpose, even commercially,
with no restriction other than to credit the original author(s).

Command Line Usage
------------------

```
msphere [options] [file_or_dir]

file_or_dir can be either a directory, or the name of a file to open, either
an SPK package or a bare JS script to execute.

--fullscreen        Starts the engine in full-screen mode.
--window            Starts the engine in windowed mode.
--frameskip <x>     Sets the initial frameskip limit to <x>. Note that if the
					game calls SetMaxFrameSkips(), that value overrides this
                    one.  The default frameskip limit is 5.
--no-throttle       Disables interframe throttling. This may improve performance
                    on slower machines at the cost of maxing out a processor
                    core.
--log-level <0-4>   Sets the console verbosity. 0 is practically silent, while 4
                    is quite noisy and useful for diagnostics. The default
                    setting is 1.

```

Potential Compatibility Issues
------------------------------

No mp3 Support
--------------

minisphere is based on Allegro, which doesn't include mp3 support due to
licensing issues. Therefore games using mp3 audio won't run in minisphere unless
the tracks are converted to another format such as Ogg Vorbis (.ogg).

`GrabImage()` and `GrabSurface()`
---------------------------------

These functions create a copy of the contents of the backbuffer and return
either an image or surface from it. In minisphere, however, this may not work as
expected due to the engine's advanced frame skipping algorithm which prevents
anything from being written to the backbuffer during a frameskip.

When writing a game specifically targetting minisphere, the correct solution is
to call `UnskipFrame()`, render what you need, then use
`GrabImage()`/`GrabSurface()` to save the render. If you absolutely *must* grab
a backbuffer image every frame (for advanced rendering effects, etc.), you can
place `SetMaxFrameSkips(0);` at the beginning of your game to prevent any frames
from being skipped.

Note that as a last resort when running older games, you can also pass
`--frameskip 0` to engine.exe on the command line to disable frame skipping.

Interframe Throttling
---------------------

minisphere attempts to minimize its CPU utilization by going to sleep while it
waits for the next frame to start. This improves battery life on laptops and
prevents the engine from heating up the CPU unnecessarily. In most cases this
causes no adverse effects; however on slower computers or more demanding games,
it may cause more frames to be skipped than necessary.

If you want to temporarily disable this feature, run engine.exe with the
`--no-throttle` command line option.

More Aggressive Frame Skipping
------------------------------

minisphere's frame skipping algorithm is very aggressive and many things that
usually happen every frame won't happen if the frame is skipped. For example,
the map engine doesn't call the render script for skipped frames. This can cause
issues if a game does anything other than rendering in a render script and can
be worked around by passing `--frameskip 0` on the command line.

Stricter Type Checking for API Calls
------------------------------------

Sphere 1.x, in general, silently coerces most wrong-type values passed to API
calls. minisphere, however, is a lot stricter about this and will throw an error
if, for example, you pass a string to a function expecting a number value. This
may prevent some poorly-written Sphere games from running under minisphere.
There is currently no workaround other than to edit the scripts by hand.
