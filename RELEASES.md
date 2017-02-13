Release Notes
=============

miniSphere X.X
--------------

* Handling of save data has been improved.  A `saveID` property must now be
  present in `game.json` and determines the subdirectory used for `~/`.  If no
  `saveID` is defined, SphereFS paths beginning with `~/` cannot be used and
  will instead throw a ReferenceError.

* `FS.openFile()` has been refactored into a `FileStream` constructor.  The
  `mode` argument of the constructor uses constants like `FileOp.Read` instead
  of the C `fopen()` mode strings used by `FS.openFile()`.  In addition,
  `FileStream#close()` has been renamed to `dispose()`.

* Games can now set the fullscreen mode programmatically by setting the
  `screen.fullScreen` to either true or false.



minisphere 4.4
--------------

* The Cellscript API has been massively overhauled in order to improve
  extensibility.  Cellscripts will need to be rewritten; review the Cellscript
  API reference to see what's changed.

* Cell's command line interface has been updated to make it easier to use.  It
  is now possible to initiate a build by simply entering `cell` while in the
  source directory.  Doing so will build your game in `./dist`.

* SphereFS sandboxing has been strengthened: Attempting to make changes in a
  location which is specified to be read-only (such as the game package) will
  now throw a TypeError.  Save data should always be stored in `~/` or a
  subdirectory.

* ECMAScript 2015 constructs such as destructuring assignment and arrow
  functions are now supported in Sphere games.  Note that Duktape doesn't
  support ES2015 syntax directly, so this requires a `transpile()` step in your
  game's Cellscript.  For convenience, the Sphere Studio project template in
  this version includes such a step already.

* All ECMAScript 2015 builtins such as WeakMap, Set, and Promise are now
  supported natively via a polyfill.  This shouldn't impact compatibility, but
  may make some things easier so it's worth bearing in mind.

* Most of the `FS` functions have been renamed to make them easier to
  understand at a glance.  For example, `FS.mkdir()` is now called
  `FS.createDirectory()`.  Scripts using these functions will need to be
  updated to work with minisphere 4.4 and later.


minisphere 4.3
--------------

* The entire engine now uses frame-perfect timing.  API calls which previously
  dealt in seconds, such as `system.now()` and `system.sleep()`, now use
  frames.  `system.sleep()` behavior has changed radically as a result - refer
  to the documentation for more information.

* The new Dispatch API allows functions to be called asynchronously from the
  event loop.  Calls may either be performed on the next tick, on a
  frame-perfect time delay, or once per frame.  Refer to the included Sphere v2
  API reference for more information.

* miniRT "struct" now provides `Reader` and `Writer` objects which work for
  both files and sockets.  Refer to the included miniRT API reference for more
  information.

* `screen.frameRate` will no longer accept `0` as a valid value.  To disable
  the frame limiter, the frame rate must now be set to `Infinity`.

* The `fs` object has been renamed to `FS` to match other namespace-like
  objects.  There's little reason for it to be treated as a concrete object
  representing the file system, and doing so limits future extensibility.

* The `mouse` and `keyboard` global variables have been removed and are now
  exposed as `Mouse.Default` and `Keyboard.Default`, respectively.  This was
  done to allow support for multiple keyboards and mice to be implemented in
  a future version without a breaking API change.

* The `ShapeGroup` constructor is now called `Model` to better reflect its
  purpose.  Other than the name change, the API remains unchanged from previous
  versions of the engine.

* The mostly redundant and non-standard `SSJ` functions have been removed from
  the API.  Going forward, the "assert" module should be used for assertions,
  and `SSJ.trace()` can be replaced with `console.trace()` with no loss of
  functionality.

* Promises are now supported natively through a polyfill.  miniRT "pact" may
  still be useful to manage promises, but this allows them to be used without
  a pact if that's desired.

* File-based objects such as `Image` and `Sound` now expose a `.fileName`
  property which allows access to the canonicalized SphereFS filename used to
  construct the object.  This may be useful in certain encapsulation scenarios.

* miniRT "threads" now takes advantage of the Dispatch API and no longer
  interferes with Sphere v1 update and render scripts.  This should help with
  migration.


minisphere 4.2
--------------

* minisphere now includes support for the Encoding API (`TextEncoder` and
  `TextDecoder` objects).  However, only UTF-8 is currently supported.  Future
  versions of minisphere will support more encodings.

* The Sphere v2 subsystem now natively supports joystick input.  Games using
  the Sphere 1.x joystick functions will continue to work as before; however,
  the new joystick API is much more flexible and should be preferred when
  writing new code.

* `kb` is now called `keyboard`.  This should make keyboard input code clearer,
  but any existing Sphere v2 code will need to be updated.


minisphere 4.1
--------------

* There is a new Standard Library module included, `assert`.  It is based on
  the Node.js built-in module of the same name, and includes most of the same
  methods with identical semantics.

* A handful of `system` object functions have been renamed and given simpler
  names.  Specifically: `system.doEvents()` is now `system.run()`, and
  `system.restart()` is now `system.reset()`.

* The Sphere Studio plugin now includes an option to display `ssj.trace()`
  output.  Previously this output would always be invisible while using the GUI
  debugger, with no way to show or even retrieve it.


minisphere 4.0
--------------

* Several object constructors such as `Image`, `Color`, and `Surface` have been
  repurposed as Sphere v2 APIs, while constructors for legacy objects such as
  `ByteArray` have been removed entirely.  Any code using the constructors will
  need to be updated.

* Objects constructed using a Sphere v2 API--such as the `Image` constructor
  (see above), `Color.Black` et al., and so on--are no longer compatible with
  the Sphere v1 API.  For example, you can't use a v2 `Color` object with
  `Rectangle()` or assign a v2 `Image` to a v1 spriteset.  Doing so will cause
  a `TypeError`.

* The Sphere v1 API is now deprecated in its entirety.  It is not recommended
  to use any Sphere v1 functions in new games, and the v1 API is thus no longer
  documented.  New games should use the Sphere v2 API exclusively and migration
  is highly recommended for existing games.

* Automatic CoffeeScript and TypeScript transpilation has been removed from the
  engine.  This was always a bit of a misfeature as Duktape isn't quite fast
  enough to do this on demand without causing unwieldy delays.  Any
  transpilation must now be done in advance.


minisphere 3.3
--------------

* Several legacy API calls have been deprecated (but retained for backward
  compatibility) in favor of new designs:

    - `Font.Default`, `Mixer.Default`, and `ShaderProgram.Default` properties
      replace `GetSystemFont()`, `GetDefaultMixer()`, and
      `GetDefaultShaderProgram()`, respectively.

    - `Color.mix()` replaces both `BlendColors` and `BlendColorsWeighted()`.

* A large collection of predefined colors (the full X11 set) is now provided
  via direct properties of `Color`.  This is often more convenient than
  constructing Color objects manually, and makes code dealing with known colors
  much more readable.

* minisphere now uses the much faster xoroshiro128+ algorithm to generate
  random numbers instead of Mersenne Twister used in past versions.  This may
  affect games using `RNG.reseed()` to manually seed the generator, since the
  generated values will differ.


minisphere 3.2
--------------

* Module IDs passed to `require()` are resolved using a more advanced algorithm
  designed to emulate the behavior of Node.js.  Compatibility is mostly
  retained; however, there may be subtle differences, for example in the
  handling of relative IDs.

* minisphere 3.2 supports `console.log()` and its variants.  Output produced
  this way will only be visible with the debugger (SSJ) attached.


minisphere 3.1
--------------

* SphereFS prefixes have changed.  Single-character prefixes are now used for
  SphereFS paths instead of the `~usr`, `~sgm`, and `~sys` aliases used in
  previous versions.  Any code depending on the old prefixes will need to be
  updated.

* The user data folder has been renamed to "minisphere".  This was done to be
  more friendly to Linux users, for whom filenames with spaces are often
  inconvenient.  If you need to keep your save data from minisphere 3.0, move
  it into `<documents>/minisphere/save`.

* The Galileo API has been updated with new features.  These improvements bring
  some minor breaking changes with them as well.  Refer to the API reference
  for details.

* The search path for CommonJS modules has changed since 3.0.  Modules are now
  searched for in `@/lib/` instead of `@/commonjs/`.

* `ListeningSocket` has been renamed to `Server`.  Networking code will need to
  be updated.


minisphere 3.0
--------------

* SphereFS sandboxing is more comprehensive.  API calls taking filenames no
  longer accept absolute paths, and will throw a sandbox violation error if one
  is given.  As this behavior matches Sphere 1.x, few if any games should be
  affected by the change.

* minisphere 3.0 stores user data in a different location than past versions.
  Any save data stored in `<documents>/minisphere` will need to be moved into
  `<documents>/Sphere 2.0/saveData` to be picked up by the new version.

* miniRT has been overhauled for the 3.0 release and is now provided in the form
  of CommonJS modules.  Games will no longer be able to pull in miniRT globally
  using `RequireSystemScript()` and should instead use `require()` to get at the
  individual modules making up of the library.  Games relying on the older
  miniRT bits will need to be updated.

* When using the SSJ command-line debugger, source code is downloaded directly
  from minisphere without accessing the original source tree (which need not be
  present).  When CoffeeScript or TypeScript are used, the JavaScript code
  generated by the transpiler, not the original source, will be provided to the
  debugger.

* CommonJS module resolution has changed.  Previously the engine searched for
  modules in `~sgm/modules`.  minisphere 3.0 changes this to `~sgm/commonjs`.

* `Assert()` behavior differs from past releases.  Failing asserts will no
  longer throw, and minisphere completely ignores the error if no debugger is
  attached.  This was done for consistency with assert semantics in other
  programming languages.

* When a debugger is attached, minisphere 3.0 co-opts the `F12` key, normally
  used to take screenshots, for the purpose of triggering a prompt breakpoint.
  This may be surprising for those who aren't expecting it.

* TypeScript support in minisphere 3.0 is mostly provisional.  In order to
  maintain normal Sphere script semantics, the TypeScript compiler API
  `ts.transpile()` is used to convert TypeScript to JavaScript before running
  the code.  While this allows all valid TypeScript syntax, some features such
  as compiler-enforced typing and module import will likely not be available.

* Official Windows builds of minisphere are compiled against Allegro 5.1.  Linux
  builds are compiled against Allegro 5.0 instead, which disables several
  features.  Notably, Galileo shader support is lost.  If a game attempts to
  construct a `ShaderProgram` object in a minisphere build compiled against
  Allegro 5.0, the constructor will throw an error.  Games using shaders should
  be prepared to handle the error.

* If SSJ terminates prematurely during a debugging session, either because of a
  crash or due to accidentally pressing `Ctrl+C`, minisphere will wait up to 30
  seconds for the debugger to reconnect.  During this time, you may enter
  `ssj -c` on the command line to connect to the running engine instance and
  pick up where you left off.
