minisphere MSVC 2015 Solution
-----------------------------

This is an MSVC solution you can use to compiling minisphere on Windows using
Visual Studio 2015 or equivalent MSBuild. Later versions of Visual Studio will
work, as well.

The solution includes x86 and x64 debug and release builds. Here is where the
engine will be built relative to the directory containing `src`:

* Console: `bin/`
* Debug:   `msvc/debug/`
* Release: `bin/`


Dependencies
------------

minisphere depends on Allegro, libpng, libmng and zlib. As building these,
especially Allegro, is time-consuming, I've included pre-built static libraries
for all Windows dependencies, which MSVC will link against when building the
solution.
