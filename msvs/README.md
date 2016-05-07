minisphere Visual Studio Solution
---------------------------------

This is a Visual Studio solution you can use to compiling minisphere on Windows
using Visual Studio 2015 or equivalent MSBuild. Later versions of Visual Studio
may work, as well.

The solution includes x86 and x64 debug and release builds. Here is where the
engine will be built relative to the directory containing `src`:

* Console - `msw/` (x86), `msw64/` (x64)
* Release - `msw/` (x86), `msw64/` (x64)
* Debug - `msvc/debug/` (x86), `msvc/debug64/` (x64)


Dependencies
------------

minisphere depends on Allegro, libmng and zlib.  On Windows, Allegro 5.2 is
provided through NuGet, and libmng and zlib are included in the repository.  As
long as you have a compatible version of Visual Studio installed, no additional
files are needed.
