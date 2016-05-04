minisphere 3.1 Install Instructions
===================================

minisphere compiles on all three major platforms (Windows, Linux, and OS X).
This file contains instructions for how to compile and install the engine on
each platform.

Windows
-------

You can build a complete 32- or 64-bit distribution of minisphere using the
included Visual Studio solution `minisphere.sln` located in `msvs/`.
Visual Studio 2015 or later is required; as of this writing,
Visual Studio Community 2015 can be downloaded free of charge from here:

[Download Visual Studio Community 2015]
(https://www.visualstudio.com/en-us/products/visual-studio-community-vs.aspx)
  
Allegro is provided through NuGet, and static libraries and/or source are included
for all other dependencies, so no additional software is required to build for
Windows.

Linux
-----

minisphere depends on Allegro 5 (5.1 or later is recommended), libmng, and zlib.
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
to the directory where you checked out minisphere and run `make` on the
command-line. This will build minisphere and all GDK tools in `bin/`. To
install minisphere on your system, follow this up with `sudo make install`.

Mac OS X
--------

Building minisphere for OS X is very similar to Linux and uses Make as well.
The dependencies are the same; however, it's not necessary to build Allegro 5.1
yourself if you have Homebrew. To get Allegro installed, simply run
`brew install --devel allegro` from the command line. Then switch to the
directory where you checked out minisphere and run `make`.
