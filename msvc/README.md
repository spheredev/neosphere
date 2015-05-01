minisphere MSVC 2013 Solution
-----------------------------

This is an MSVC solution you can use for compiling minisphere on Windows
using Visual Studio 2013 or equivalent MSBuild. Later versions of
Visual Studio may work, as well.

The solution includes x86 and x64 debug and release builds. Here is
where the engine will be built relative to the directory containing
`src`:

* Console: `bin/`
* Debug:   `msvc/debug/`
* Release: `bin/`


*NOTE:* As building Allegro and its dependencies on Windows is extremely
time-consuming, especially if you want the x64 builds, I've included
pre-built static libraries for Allegro 5.1.9, which MSVC will link
against when building the solution.
