This is an MSVC solution you can use to compile minisphere on Windows
using Visual Studio 2013 or higher or equivalent MSBuild.

The solution includes x86 and x64 debug and release builds. Here is
where the engine will be built relative to the directory containing
'src':

Release - bin/
Debug - msvc/debug/


NOTE: As building Allegro and its dependencies is extremely time-
consuming in Windows, especially if you want the x64 builds, I've
included pre-built static libraries for Allegro 5.1.9 which MSVC will
link against when building the solution.
