minisphere 1.0.8
================

*Friday, April 10, 2015*

minisphere is a drop-in replacement for the Sphere game engine written
from the ground up in C.  It boasts high compatibility with most games
written for Sphere 1.x, but with better performance and new
functionality.

minisphere is also very small, living up to its name: The engine in its
entirety consists of a single executable less than 2MB in size, plus
around 200KB or so for the system assets. This makes it very easy to
distribute compared to Sphere 1.x, which depends on about 7 DLL files
and won't run at all if any of them are left out.


Command Line Options
--------------------

* `--game <path>`: Loads the Sphere game at `<path>`. If not provided,
  minisphere will open a file dialog allowing the user to locate a
  desired game manually. For compatibility reasons, this can also be
  specified as `-game <path>` (with only one dash).

* `--fullscreen`: Starts the engine in full-screen mode.

* `--windowed`: Starts the engine in windowed mode.

* `--frameskip <x>`: Sets the initial frameskip limit to `<x>`. Note
  that if the game calls `SetMaxFrameSkips()`, that value overrides this
  one.  The default frameskip limit is 5.

* `--no-throttle`: Disables interframe throttling. This may improve
  performance on slower machines at the cost of maxing out a processor
  core.


Potential Compatibility Issues
------------------------------

No mp3 support in minisphere
----------------------------

minisphere is based on Allegro, which doesn't include mp3 support due
to licensing issues. Therefore games using mp3 audio won't run in
minisphere unless the tracks are converted to another format such as
Ogg Vorbis (.ogg).

`GrabImage()` and `GrabSurface()`
---------------------------------

These functions create a copy of the contents of the backbuffer and
return either an image or surface from it. In minisphere, however, this
may not work as expected due to the engine's advanced frame skipping
algorithm which prevents anything from being written to the backbuffer
during a frameskip.

When writing a game specifically targetting minisphere, the correct
solution is to call `UnskipFrame()`, render what you need, then use
`GrabImage()`/`GrabSurface()` to save the render. If you absolutely
*must* grab a backbuffer image every frame (for advanced rendering
effects, etc.), you can place `SetMaxFrameSkips(0);` at the beginning
of your game to prevent any frames from being skipped.

Note that as a last resort when running older games, you can also pass
`--frameskip 0` to engine.exe on the command line to disable frame
skipping.

Interframe throttling
---------------------

minisphere attempts to minimize its CPU utilization by going to sleep
while it waits for the next frame to start. This improves battery life
on laptops and prevents the engine from heating up the CPU
unnecessarily. In most cases this causes no adverse effects; however on
slower computers or more demanding games, it may cause more frames to be
skipped than necessary.

If you want to temporarily disable this feature, run engine.exe with the
`--no-throttle` command line option.

Aggressive frame skipping
-------------------------

minisphere's frame skipping algorithm is very aggressive and many things
that usually happen every frame won't happen if the frame is skipped.
For example, the map engine doesn't call the render script for skipped
frames. This can cause issues if a game does anything other than
rendering in a render script and can be worked around by passing
`--frameskip 0` on the command line.

Stricter type checking in API functions
---------------------------------------

Sphere 1.x, in general, silently coerces most wrong-type values passed
to API calls. minisphere, however, is a lot stricter about this and will
throw an error if, for example, you pass a string to a function
expecting a number value. This may prevent some poorly-written Sphere
games from running under minisphere. There is currently no workaround
other than to edit the scripts by hand.

`const` will cause a syntax error
---------------------------------

Duktape, the JavaScript engine that powers minisphere, is fully ES5
compliant and will throw a syntax error if the `const` keyword is
encountered. If you want to try running the game in minisphere, you will
need to manually replace all instances of `const` in the game's scripts
with `var`.
