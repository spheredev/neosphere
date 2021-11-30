neoSphere Release Checklist
===========================

Prepare the Release
-------------------

The following changes should be made in a separate commit.  The first line of
the commit message should be `neoSphere X.Y.Z`, where `X.Y.Z` is the version
number of the release, and a tag `vX.Y.Z` should be created which points to
this new commit.

* Version number in `VERSION`
* Version number in Win32 resources (`msvs/*.rc`)
* Version number in `src/shared/version.h`
* Version number and release date in `README.md`
* Version number and release date in manual pages (`manpages/*`)
* Version number, release date, and changelog entries in `CHANGELOG.md`
* Version number in `setup/neoSphereSetup.iss`


Build the Release
-----------------

* In Windows using Visual Studio 2019:
    - Run `git clean -xdf`, then build the following project configurations:
        + **neoSphere:** x64 Redist, x64 Console, Win32 Redist, Win32 Console
        + **Cell:** x64 Console, Win32 Console
        + **SSj:** x64 Console, Win32 Console
        + **Plugin:** AnyCPU Release
    - Compile `setup/neoSphereSetup.iss` using the latest version of Inno Setup
    - `neoSphereSetup-X.Y.Z-msw.exe` will be in `setup/`

* Using a 64-bit installation of Ubuntu:
    - Run `make clean all dist`
    - `neosphere_X.Y.Z.tar.gz` will be in `dist/`


Unleash the Beast!
------------------

* Drink a bunch of Monster drinks, at least 812 cans, and then...

* Post a release to GitHub pointing at the new Git tag, and upload the
  following files built above:
    - `neoSphereSetup-X.Y.Z-msw.exe`
    - `neosphere-X.Y.Z.tar.gz`
