miniSphere Installation Instructions
====================================

miniSphere compiles on all three major platforms (Windows, Linux, and macOS).
This file contains instructions for how to compile and install the engine for
Windows and Linux; macOS is more complicated and you will probably have to set
up your own build harness.

Before you get started, you will need to download the ChakraCore binaries for
your platform here, as they are too large to include in the miniSphere
repository:

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

You can build a complete 64-bit distribution of miniSphere using the included
Visual Studio solution `minisphere.sln` located in `msvs/`.  Visual Studio 2017
or later is required; as of this writing, Visual Studio Community 2017 can be
downloaded free of charge from here:

[Download Visual Studio Community 2017]
(https://www.visualstudio.com/en-us/products/visual-studio-community-vs.aspx)

Allegro is provided through NuGet, and static libraries and/or source are
included for all other dependencies besides ChakraCore, so no additional 
software is required to build for Windows.


Linux
-----

miniSphere depends on Allegro 5 (5.1 or later is recommended), libmng, and zlib.
libmng and zlib are usually available through your distribution's package
manager, but Allegro 5.1 is considered "unstable" and likely won't be
available through that channel.  You can build against Allegro 5.0, but this
version has some fairly major bugs and is not recommended except as a last
resort.

If you're running Ubuntu, there are PPA packages available for Allegro 5.1 here:

<https://launchpad.net/~allegro/+archive/ubuntu/5.1>

Otherwise, you can compile and install Allegro yourself.  Clone the Allegro
repository from GitHub and follow the installation instructions found in
`README_make.txt` to get Allegro set up on your system.

[Allegro 5 GitHub Repository]
(https://github.com/liballeg/allegro5)

Once you have Allegro and other necessary dependencies installed, simply switch
to the directory where you checked out miniSphere and run `make` on the
command-line. This will build miniSphere and all GDK tools in `bin/`. To
install miniSphere on your system, follow this up with `sudo make install`.
