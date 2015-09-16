Cell v2.0-WIP
=============

Cell is a scriptable compiler for Sphere games. Like minisphere, it uses
JavaScript to control the compilation process and is based on Duktape.


Command Line Options
--------------------

* `--version`: Prints version information and exits.

* '--make-package', '-p': Creates a Sphere SPK package in the output directory
  from the compiled game.

* `--out <pathname>`: Sets the directory where the game will be compiled. If
  this is a relative path, it is relative to the current directory. The default
  output directory is "dist".
