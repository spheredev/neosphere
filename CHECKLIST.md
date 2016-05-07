minisphere Release Checklist
============================

Prepare the Release
-------------------

The following changes should be made in a separate commit.  The first line of
the commit message should be `minisphere X.Y.Z`, where `X.Y.Z` is the version
number of the release, and a tag `vX.Y.Z` should be created which points to this
new commit.

* Version number in `VERSION`
* Version number in Win32 resources (`msvs/*.rc`)
* Version number in `src/shared/version.h`
* Version number in `src/plugin/PluginMain.cs`
* Version number in `src/plugin/Properties/AssemblyInfo.cs`
* Version number in `docs/spherical-api.txt`
* Version number in `setup/minisphere.iss`
* Version number and release date in `README.md`
* Version number and release date in manual pages (`manpages/*`)
* Release date in CHANGELOG.md
* Changelog entry in `src/debian/changelog`

Build the Release
-----------------

* In Windows using Visual Studio 2015:
    - Delete `msw/` and `msw64/`, then Clean and Build the following
      configurations:
        + **minisphere:** x64 Redist, x64 Console, Win32 Redist, Win32 Console
        + **Cell:** x64 Console, Win32 Console
        + **SSJ:** x64 Console, Win32 Console
        + **Plugin:** AnyCPU Release
    - Copy latest Sphere Studio "Release" build into `msw/ide/`
    - Compile `setup/minisphere.iss` using Inno Setup 5.5
    - `minisphereSetup-X.Y.Z.exe` will be in `setup/`
* Using a 64-bit installation of Ubuntu 14.04:
    - Run `make clean dist deb && cd dist`
    - Run `pbuilder-dist trusty minisphere_X.Y.Z.dsc`
    - Run `pbuilder-dist trusty i386 minisphere_X.Y.Z.dsc`
    - Run `dput ppa:fatcerberus/minisphere minisphere_X.Y.Z_source.changes`
    - `minisphere_X.Y.Z.tar.gz` will be in `dist/`
    - `minisphere_X.Y.Z_amd64.deb` will be in `~/pbuilder/trusty_result/`
    - `minisphere_X.Y.Z_i386.deb` will be in `~/pbuilder/trusty-i386_result/`

Unleash the Beast!
------------------

* Drink a bunch of Monster drinks, at least 812 cans, and then...
* Post a release to GitHub pointing at the new Git tag, and upload the following
  files built above:
    - `minisphereSetup-X.Y.Z.exe`
    - `minisphere-X.Y.Z.tar.gz`
    - `minisphere_X.Y.Z_amd64.deb`
    - `minisphere_X.Y.Z_i386.deb`
