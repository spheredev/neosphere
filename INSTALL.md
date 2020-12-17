Sphere Installation Instructions
================================

Sphere compiles on all three major platforms (Windows, Linux, and macOS).  This
file contains instructions for how to compile and install Sphere for Windows
and Linux; macOS is more complicated and you will probably have to set up your
own build harness.

Before you get started, you will need to download the ChakraCore binaries for
your platform here, as they are too large to include in the Sphere repository:

https://github.com/Microsoft/ChakraCore/releases

Copy `ChakraCore.lib` (and for Windows builds, `ChakraCore.dll`) into `dep/lib`
and the following header files into `dep/include`:

* `ChakraCommon.h`
* `ChakraCommonWindows.h`
* `ChakraCore.h`
* `ChakraCoreVersion.h`
* `ChakraCoreWindows.h`
* `ChakraDebug.h`


Windows
-------

You can build a complete 64-bit distribution of Sphere using the included
Visual Studio solution `sphere.sln` located in `msvs/`.  Visual Studio 2019 or
later is required; as of this writing, Visual Studio Community 2019 can be
downloaded free of charge from here:

[Download Visual Studio Community 2019]
(https://www.visualstudio.com/en-us/products/visual-studio-community-vs.aspx)

Allegro is provided through NuGet, and static libraries and/or source are
included for all other dependencies besides ChakraCore, so no additional 
software is required to build for Windows.


Linux
-----

neoSphere depends on Allegro 5, libmng, and zlib.  All of these are usually
available through your distribution's package manager.

Once you have Allegro and other necessary dependencies installed, simply switch
to the directory where you checked out Sphere and run `make` on the command
line.  This will build neoSphere and all GDK tools in `bin/`. To install Sphere
on your system, follow this up with `sudo make install`.
