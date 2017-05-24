miniSphere Changelog
====================

vX.X.X - TBD
------------

* Adds a new `Sample` class for loading sounds into memory instead of streaming
  them.  This may reduce latency, and allows the same sound to be played back
  multiple times simultaneously.
* Adds support for `Sphere.Game` in Cellscripts, allowing direct modification
  of the JSON manifest.
* Updates the "term" module to use `Dispatch` internally instead of Threads.
* Removes `describe()` from the Cellscript API.
* Fixes a bug where `Dispatch.onRender()` jobs are called in reverse order
  following a call to `Dispatch.onUpdate()`.
* Fixes a bug where Sphere v1 `Surface#cloneSection()` and `Surface#rotate()`
  could introduce graphical artifacts on some platforms.
* Fixes a bug where Cell will sometimes not detect a filename conflict.


v4.5.11 - April 27, 2017
------------------------

* Now uses TypeScript to transpile Cellscripts, improving build times a bit.
* Fixes a bug where SSJ repeats the previous action after an invalid command.
* Fixes a bug where regular expressions containing an octal escape (e.g. `\3`)
  cause a SyntaxError.

v4.5.10 - April 3, 2017
-----------------------

* Fixes a bug where Sphere v1 `drawZoomedText()` ignores non-opaque pixels.
* Fixes a bug in the RFN font loader where RFNv1 (grayscale) fonts are loaded
  without an alpha channel.

v4.5.9 - March 31, 2017
-----------------------

* Adds tolerance for shebang lines (`#!...`) at the start of JS source files.
* TypeScript is once again used as the default ES6 transpiler.  A source map is
  embedded in each transpiled script that is used by SSJ Blue to facilitate
  debugging.

v4.5.8 - March 20, 2017
-----------------------

* Add `from#reduce()` API for reducing a collection to a single value.
* Fixes spurious "target file unchanged after build" warnings when a target
  fails to build due to a compile-time error.
* Fixes a bug where Cell doesn't delete target files that fail to build.
* Fixes a Duktape bug where redeclaring an existing global binding as a `var`
  in global code resets its value to `undefined`.

v4.5.7 - March 14, 2017
-----------------------

* Adds a new `Thread` class which can be subclassed in ES6 to make threaded
  entities.
* Changes `from#select()` to be lazy and removes `from#mapTo()` which is now
  redundant.  If you need to take a snapshot of the current query results,
  `.toArray()` can now be used for that purpose. 
* Fixes an issue where `.mjs` files are not renamed to `.js` when transpiling.
* Fixes various bugs in the Sphere Studio project template.

v4.5.6 - March 11, 2017
-----------------------

* Improves `for...of` iteration for `from()` queries.  Only the values will now
  be enumerated, rather than key/value pairs.
* Reverts to Babel as the default ES6 transpiler pending source map support.
* Fixes a bug where a CommonJS module without a trailing newline and with a
  C++-style comment (`//`) on the last line causes `require()` to throw an
  erroneous `SyntaxError`.

v4.5.5 - March 10, 2017
-----------------------

* Changes the built-in ES6 transpiler (used by Cell's `compiler` module) to
  TypeScript, vastly improving compile times over Babel.
* Fixes a bug where promise settlement causes an unexpected `TypeError`.
* Fixes a bug where texture coordinates are not normalized for a `Shape` drawn
  directly to an off-screen surface (this also affected `prim.blit()`).

v4.5.4 - March 5, 2017
----------------------

* Adds a new global binding, `exports`, which maps to the global object, to
  better support certain transpilers (e.g., TypeScript).
* Adds `for...of` support for `from()` queries.  Query results are enumerated
  as key/value pairs (`.v`, `.k`).
* Adds `from.iterable()` for querying ES6 iterables.
* Renames the Cell "transpile" module to "compiler" and makes `transpile()` a
  named export.


v4.5.3 - February 28, 2017
--------------------------

* Adds support for `Cellscript.mjs`.  if `Cellscript.js` is used, it will now
  be compiled as a script rather than a module.
* Adds `.mjs` module support for Cellscript `require()`.
* Fixes a bug in `from.Object()` where no object properties are enumerated.
* Fixes a bug where `term.print()` repeats its second parameter for every one
  beyond that.

v4.5.2 - February 25, 2017
--------------------------

* Changes Cell `transpile` module to use an automatic mode for `transpile()`.
  `.js` files are compiled as scripts (which is Sphere 1.x compatible), while
  `.mjs` files are compiled as ES6 modules.  This is in line with how Node.js
  will eventually implement ES6 module support.

v4.5.1 - February 22, 2017
--------------------------

* Adds `PurwaBlue` and `RebeccaPurple` as predefined colors.

v4.5.0 - February 20, 2017
--------------------------

* Adds support for game-specific save directories.  Set `saveID` in a game's
  JSON manifest to control the subdirectory to which `~/` is mapped.
* Adds a new `FileStream` constructor, which replaces `FS.openFile()` and has
  a more intuitive API.
* Adds a `screen.fullScreen` property to change the fullscreen mode at runtime.
* Adds a compile-time warning for Cell targets with no sources (the tool will
  never be invoked in that case).
* Adds support to Cell for SGMv1 manifest generation, allowing limited
  cross-compatibility with Sphere 1.x to help ease migration.
* Renames the `Image` class to `Texture` to support future compatibility with
  Oozaru.
* Renames `system.now()` to `screen.now()`.
* Renames the `system` object to `Sphere`, and fixes capitalization for runtime
  constants, e.g., `system.version` -> `Sphere.Version`.
* Renames the `--window` command-line option to `--windowed`.
* Changes `Color#name` to return InterCaps names for predefined colors, e.g.
  "DodgerBlue" instead of "dodgerblue".
* Changes `Sphere.sleep()` to use its pre-4.3 behavior, taking a value in
  seconds and suspending all engine activity, including rendering, until the
  timeout expires.
* Fixes a bug where calling `FileStream` methods on a disposed stream can cause
  miniSphere (and Cell) to crash.


v4.4.4 - February 6, 2017
-------------------------

* The engine is now called miniSphere, with a capital 'S', both to highlight
  its nature as a JavaScript-powered engine and to make the name more readily
  recognizable.
* Adds `for...of` enumeration support for `from()` queries.
* Improves color coding of SSJ output to give a better picture of what's going
  on.  Engine state is highlighted in cyan; debugger state is highlighted in
  gold.
* Improves Symbol support by enabling native Symbols in Duktape.
* Improves the Sphere Studio project template: the sample Cellscript now uses
  `import` to pull in the transpile function, showcasing Cell's new built-in
  ES 2015 support.

v4.4.3 - February 1, 2017
-------------------------

* Adds support for ECMAScript 2015 syntax in Cellscripts and modules.
* Adds a new API function, `Color.is()`, for comparing Colors without regard to
  their alpha channels.
* Fixes a bug where `this` is bound to the global object in top-level module
  code instead of the `module` object like Node.js.
* Fixes a bug where `require()` incorrectly picks up files with `.ts` or
  `.coffee` extensions.

v4.4.2 - January 30, 2017
-------------------------

* Fixes a bug where specifying a nonexistent directory in a Cellscript
  `files()` directive crashes the compiler.
* Fixes a bug where `transpile.v1()` parses input files in strict mode, causing
  some otherwise valid Sphere v1 legacy code to be rejected with a syntax
  error.
* Fixes a bug which causes Cell to produce a spurious "target file not built"
  error if a Tool throws an exception.

v4.4.1 - January 28, 2017
-------------------------

* Fixes a bug which caused the Arch and Ubuntu installations of Cell to be
  unable to find the standard library modules.
* Fixes a bug which caused minisphere to crash on startup when trying to load
  a malformed Sphere v2 manifest.

v4.4.0 - January 27, 2017
-------------------------

* Overhauls the Cellscript API: `require()` is supported, file paths are
  resolved using SphereFS semantics, and Cellscripts and modules can now define
  custom targets and build tools for the ultimate in extensibility.
* Improves the naming of the `FS` functions to make their intent clearer.  For
  instance, `FS.mkdir()` -> `FS.createDirectory()`.
* Strengthens the SphereFS sandbox.  Only the user directory (`~/`) is now
  writable; passing any other path to an API that requires write access will
  result in a TypeError.
* Simplies the command-line interface for Cell.  You can now initiate a build
  simply by running `cell` (with no additional options) from any directory with
  a Cellscript in it.
* Adds `FS.readFile()` and `FS.writeFile()` which allow reading and writing
  an entire file in a single operation.
* Adds a `screen.frameSkip` property to allow games to control frame skipping.
* Adds support for ECMAScript 2015 built-ins like Map and Set using a polyfill
  which is loaded automatically on startup.
* Adds a `--clean` command line option to have Cell delete all artifacts from
  the most recent build.
* Adds a `--rebuild` command line option to have Cell build all targets, even
  if they are already up to date.
* Adds support to Cell for ECMAScript 2015+ transpilation via Babel.
* Adds support to Cell for minification via Babili.
* Improves Sphere 1.x compatibility in various corner cases, and adds several
  missing Sphere v1 APIs.
* Dispatch API functions will no longer accept a JavaScript source code string
  as a callback; the callback must now always be a function.
* Improves the miniRT API documentation.
* Fixes ANSI text coloration for SSJ in Windows 10 Anniversary Update and
  later.
* Fixes an issue where Cell doesn't automatically rebuild a game if the
  Cellscript is modified between runs.
* Fixes a bug where CommonJS module filenames don't get mapped to their source
  names when using SSJ Blue, causing the debugger to not be able to find them.
* Fixes a bug where the miniRT terminal can get rendered before other threads.
* Fixes a bug where `random.sample()` sometimes returns `undefined`.
* Fixes a bug where error attribution doesn't work properly if `Duktape.act()`
  is not available at the time of the error.
* Fixes a bug where a non-Error exception thrown from global code produces a
  confusing "script not found" error.
* Fixes a bug where `CreatePerson()` won't accept a Spriteset object.


v4.3.8 - December 4, 2016
-------------------------

* Fixes a bug which caused `from#anyIn()` to always return `false`.

v4.3.7 - December 2, 2016
-------------------------

* Adds experimental support for ECMAScript 2015 `Symbol()` and `Symbol.for()`.
* Adds a bunch of new query operators: `shuffle()`, `random()`, `sample()`,
  `including()`, `ascending()`, `descending()`, and `from()`.

v4.3.6 - November 23, 2016
--------------------------

* Adds support for composable query operators, allowing partial application of
  `from()` queries, like in LINQ.
* Renames from#map() to from#mapTo().


v4.3.5 - November 20, 2016
--------------------------

* Adds `from#allIn()`, `from#anyIn()`, and `from#anyIs()`, providing a quick
  way to check whether a query matches specific values without the need to
  write a predicate function.
* Adds `from#first()` and `from#last()` to get the very first or very last
  result of a query, respectively.
* Adds support for `Math.cbrt()`, `Math.log2()`, `Math.log10()`, and
  `Math.trunc()`.

v4.3.4 - November 17, 2016
--------------------------

* Adds a new miniRT module, "from", which enables performing queries against
  the contents of objects, arrays, and strings.  This replaces the undocumented
  third-party "link" module which was included with previous versions of the
  engine.
* Adds support for ECMAScript 2015 `Object.is()` and `Math.hypot()`.
* Fixes a bug where `Color.of()` was undefined and not callable.

v4.3.3 - November 11, 2016
--------------------------

* Adds a new miniRT "test" module based on the CommonJS Unit Testing/1.0
  specification.
* Adds support for ECMAScript 2015 `Object.assign()`.
* Fixes `Abort()` to exit the game unconditionally, like Sphere 1.x.
* Fixes `ExecuteGame()` resetting the fullscreen/window state.

v4.3.2 - November 9, 2016
-------------------------

* Adds an API call, `Dispatch.cancelAll()` to cancel all pending one-time jobs
  (recurring jobs must still be cancelled individually).
* Adds support for ECMAScript 2015 binary (`0b1000`) and octal (`0o1454`)
  notation.
* Improves the Sphere Studio project template to show off more features and
  better illustrate how the Sphere v2 API works.
* Improves Dispatch API priority handling.  Update jobs are now executed in
  order from highest to lowest priority, rather than the other way around.
* Fixes jobs continuing to run after an uncaught exception.
* Fixes uncaught exceptions within the central frame loop crashing the engine
  instead of displaying the error screen.

v4.3.1 - November 8, 2016
-------------------------

* Improves Sphere v2 behavior: minisphere will now run a frame loop until there
  are no more Dispatch API jobs active before exiting.

v4.3.0 - November 7, 2016
-------------------------

* Adds a new Dispatch API for setting up asynchronous jobs.  Jobs are called
  from the event loop; for example, `Dispatch.onUpdate()` is processed at the
  start of every frame.
* Adds support for the JavaScript Reflect API.
* Adds native support for JavaScript Promises via a polyfill.
* Adds `struct.Reader` and `struct.Writer` which allow games to read and write
  binary data from files and network sockets.
* Adds a `.fileName` property to objects such as Image and Sound, which returns
  the canonicalized name of the file the object was loaded from.
* Adds `assert()` as an alias for `assert.ok()`, like in Node.js.
* Renames the `fs` global object to `FS` to match other namespace-like objects.
* Renames `mouse` to `Mouse.Default` to allow future support for multiple mice.
* Renames `keyboard` to `Keyboard.Default` to allow future support for multiple
  keyboards.
* Renames `ShapeGroup` to `Model`, better describing the object's purpose.
* Removes the superfluous `SSJ` functions from the API.
* Fixes several bugs discovered since minisphere 4.2.4 was released.
* Updates the Sphere v2 API to use frame-perfect timing throughout.  For
  example, `system.now()` returns the number of frames processed since engine
  start.
* Improves Sphere 1.x compatibility.  Many games which historically didn't run
  properly in minisphere, including Kefka's Revenge and Trial and Error, should
  now be fully playable.
* Updates error messages to be more concise, making it clearer what went wrong
  at a quick glance.
* miniRT no longer commandeers the legacy update and render scripts for itself.
* When debugging with Sphere Studio, the gold "ssj" watermark in the bottom
  right is changed to blue to more easily distinguish interactive debugging
  sessions from command-line sessions.


v4.2.4 - October 7, 2016
------------------------

* Adds streaming support for `TextDecoder`.
* `system.abort()` now aborts unconditionally, as described in the API
  documentation.
* Fixes a bug in the UTF-8 validation logic that caused Unicode strings to be
  double-encoded, leading to undesirable side effects and in rare cases, a
  crash.

v4.2.3 - October 6, 2016
------------------------

* Adds support for the new JavaScript exponentiation operator (e.g. `x ** 2`),
  allowing powers to be calculated without the performance impact of a
  `Math.pow()` call.
* `term.log()` is renamed to `term.print()`.
* Updates the Encoding API implementation to more closely follow the algorithms
  described in the WHATWG standard.

v4.2.2 - September 28, 2016
---------------------------

* Adds a new property, `term.visible` to the `term` module and removes the
  `.show()` and `.hide()` methods.
* Fixes the `struct` and `logger` modules, which were unintentionally broken by
  the changes in 4.2.0.

v4.2.1 - September 21, 2016
---------------------------

* The `ssj` global is now called `SSJ`, to align with usual JavaScript naming
  conventions for namespace-like objects.
* The `terminal` standard library module is now called `term`.
* Passing an invalid argument to a miniRT `random` module function will now
  result in a hard `AssertionError`, rather than just a debugger trap.

v4.2.0 - September 14, 2016
---------------------------

* Adds native joystick support to the Sphere v2 API.  Previously, using a
  joystick or gamepad required calling Sphere v1 legacy functions.
* Adds a new module to miniRT, `joy`.  This provides a simplified joystick
  interface for games that don't need the full flexibility of the Sphere v2
  Joystick API.
* Adds W3C Encoding support (`TextEncoder` and `TextDecoder` objects) to the
  Sphere v2 API.  This allows reading and writing text from ArrayBuffer views.
* Simplifies the FileStream API.  All methods for reading and writing typed
  values from files have been removed, leaving only `read()` and `write()`
  which deal with ArrayBuffers directly.
* The `kb` built-in object is now called `keyboard`, making keyboard input code
  significantly easier to read at the cost of some extra verbosity.
* Fixes a long-standing map engine bug where the input person(s) can't be moved
  with the joystick.


v4.1.1 - August 24, 2016
------------------------

* When starting a game with a JSON manifest, the main script is now run as a
  CommonJS module unless the manifest explicitly specifies `version: 1`.
* `console` methods now actually send text to the console.
* Sphere Studio now runs minisphere Console at V0 verbosity by default.

v4.1.0 - August 22, 2016
------------------------

* Adds a new module to the standard library: `assert`, based on the Node.js
  built-in assert module.  Unlike `ssj.assert()`, the functions in this module
  throw an `AssertionError` when an assertion fails.
* Renames a few `system` APIs, giving them easier-to-remember names.
* Adds an option to show `ssj.trace()` output while debugging a game with
  Sphere Studio.


v4.0.1 - August 14, 2016
------------------------

* `system.extensions` now includes properties for individual extensions.  Each
  has a value of `true`, which allows games to check for extensions via, e.g.
  `if (system.extensions.sphere_stateful_rng) { ... }`.
* Fixes a bug which caused the `system.extensions` array to be missing entries.

v4.0.0 - August 12, 2016
------------------------

* Introduces the new Sphere v2 API.  The entire Sphere v1 API, including the
  map engine, has been deprecated and is no longer recommended for use in new
  games.  For information on using the functions in the new API, see
  `docs/sphere2-api.txt`.
* miniRT has been overhauled and repurposed as minisphere's implementation of
  the Sphere v2 standard library.  See `docs/miniRT-api.txt` for more
  information.
* Drops support for on-the-fly transpilation of TypeScript and CoffeeScript.
  This must now be done in a separate step.


v3.3.0 - May 31, 2016
---------------------

* Now uses xoroshiro128+ as the random number generator instead of the slower
  Mersenne Twister.
* Adds a new property to the RNG object, `RNG.state`.  This allows you to save
  and restore the internal state of the random number generator, for example to
  deter save scumming.
* Adds a bunch of new API calls to make working with Color objects easier:
  `Color.fade()`, `Color.of()`, `Color.mix()` (which supersedes the old
  blending functions), `Color#name`, and a bunch of predefined colors such as
  `Color.Chartreuse`, `Color.Blue`, etc.
* `Font`, `Mixer`, `ShaderProgram` and `WindowStyle` now include static
  `.Default` properties which refer to the corresponding built-in default
  assets.


v3.2.0 - May 22, 2016
---------------------

* The module system has been overhauled to work more like Node.js, and now has
  support for `package.json` files, parsing JSON files as objects, and useful
  APIs such as `require.cache`, `module.loaded`, and `module.require`.
* `require()` can now load JS modules from anywhere in a game's file system by
  prefixing the module ID with a SphereFS alias (`@/`, `~/`, or `#/`).
* Adds support for `console.log()` and friends.  `stdout` is reserved for
  under-the-hood logging, so `console` output will only be visible when SSJ is
  attached.
* Improves frameskip behavior: The frame timer is now reset only if the skip
  limit is hit.  This should ensure a more consistent update rate even under
  lag conditions.


v3.1.2 - May 11, 2016
---------------------

* Fixes an issue where sounds were reinitialized whenever a script called
  `Sound#play()` (including the first time), potentially triggering a deadlock
  bug in Allegro 5.0.  This affects Linux only, as Allegro 5.2 is used on
  Windows.
* Fixes a regression in minisphere 3.1.0 which could cause games with an
  otherwise valid `.s2gm` manifest to fail to start with an Unsupported Engine
  error.

v3.1.1 - May 10, 2016
---------------------

* Fixes a bug in miniRT/music where it tried to call `console.write()` instead
  of `console.log()`, causing it to throw an error and potentially crash the
  game when logging error messages.
* `Sound#play()` will now accept a Mixer as its second or third argument.
  Previously the Mixer had to be passed to the Sound constructor, which was
  awkward.
* The map engine now uses its own mixer for map-defined BGM.

v3.1.0 - May 7, 2016
--------------------

* SphereFS now uses single-character aliases: `#/` for built-in engine assets,
  `@/` for the root of the sandbox, and `~/` for the user data directory (for
  save data).
* Changes the user data directory name back to "minisphere" to be more friendly
  to Linux users.
* Adds some new components to miniRT: miniRT/binary for easy loading of
  structured binary data, miniRT/xml for XML parsing and DOM generation, and
  miniRT/prim to pre-render expensive-to-draw primitives like circles.
* Adds a new Transform object which allows working with transformation
  matrices.
* Improves the Galileo API: Shapes can now be drawn directly, Groups have a
  `transform` property which allows their transformation matrices to be
  manipulated, and shader uniforms can be set using `group.setInt()`,
  `group.setFloat()`, and `group.setMatrix()`.
* Adds new Galileo Shape types `SHAPE_LINE_LOOP` and `SHAPE_LINE_STRIP`.
* minisphere now looks for CommonJS modules in `lib/` instead of `commonjs/`.
* `Async()` is now called `DispatchScript()` for API consistency.
* `ListeningSocket` is now called `Server`.
* You can now use `-0` through `-4` on the command line to specify the engine
  log verbosity level.


v3.0.8 - April 17, 2016
-----------------------

* Fixes a bug where minisphere would crash instead of showing an error message
  if it was unable to create a render context.
* SSJ will now continue with the previous course of action if given a null
  command.  This only works for certain commands.

v3.0.7 - April 14, 2016
-----------------------

* Fixes an issue where persons with large ignore lists would cause an inordinate
  amount of lag.  This was caused by the engine checking persons' ignore lists
  before their hitboxes.

v3.0.6 - April 11, 2016
-----------------------

* Reverts to the pre-3.0.4 method of map rendering.  It turns out that Allegro's
  sprite batcher is actually pretty awesome.

v3.0.5 - April 10, 2016
-----------------------

* Fixes a performance regression in 3.0.4 related to Galileo map rendering and
  animated tilesets.

v3.0.4 - April 9, 2016
----------------------

* Fixes a memory management bug in Galileo which caused it to leak potentially
  massive amounts of memory in games which generate a lot of `Shape` objects.
* Fixes a bug in the Windows build where `stdout` couldn't be redirected.
* Updates the map engine to use the Galileo graphics subsystem internally, which
  improves rendering performance in most cases.
* Fixes a segfault when rendering a Galileo primitive with no vertices defined.

v3.0.3 - April 5, 2016
----------------------

* While debugging in Sphere Studio, variables are now sorted alphabetically in
  the Debugger pane.
* Fixes a bug where `GetDefaultShaderProgram()` would attempt to compile the
  same source for both the vertex and fragment shaders, causing the call to
  fail.
* Implements `RNG.random()`, an API function which has been documented for a
  while without actually being present.

v3.0.2 - April 1, 2016
----------------------

* Improves the file API: The `FileStream` object now includes methods for
  directly reading and writing integers, strings, and floats in addition to the
  standard `ArrayBuffer`-based I/O.
* The Windows build now uses Allegro 5.2.0, the latest stable version.
* Fixes a bug in the Sphere Studio debugger where pressing F10 would perform a
  Step Out instead of Step Over.

v3.0.1 - March 29, 2016
-----------------------

* Fixes a bug where running `minisphere` from the command line and giving an
  invalid or nonexistent game path would cause the engine to segfault trying to
  display an error message.
* Adds Sphere 1.x API functions `SetLayerWidth()` and `SetLayerHeight()`.  For
  convenience, I also added `SetLayerSize()` to set both dimensions at once.
* In Sphere Studio, the Debugger pane is now hidden when not actively debugging.
  This helps maximize screen real estate without forcing the user to set the
  pane to autohide.
* Fixes a bug where receiving a malformed source code request from the debugger
  would cause a segfault.  This wasn't a security risk right now, but might have
  become one in the future once I add remote debugging support.

v3.0.0 - March 28, 2016
-----------------------

* The Windows redistributable and GDK downloads have been combined into a single
  installer.  The engine is so compact that there's nothing gained from having
  separate installers.
* minisphere is now officially supported on Linux! `.deb` binary and `.tar.gz`
  source packages will be provided for all minisphere releases going forward.
* miniRT is completely revamped and modernized.  All miniRT components have been
  rewritten as CommonJS modules which allows them to be pulled in individually
  as needed, instead of all at once using a global RequireScript().
* A complete API reference for miniRT is now included with the engine.
* Introducing the brand-new command-line debugger, SSJ!  SSJ can be started by
  running `ssj <game-path>` on the command line.  This brings single-step Sphere
  debugging to non-Windows platforms for the first time!
* Strengthens the SphereFS sandbox: Using absolute file paths is no longer
  supported and will result in a sandbox violation error.
* Adds provisional TypeScript support.  minisphere uses `ts.transpile()`
  internally to convert TypeScript to JavaScript, so some TypeScript features
  may not work as expected.  See the release notes for more details.
* User data (screenshots, save files, etc.) is now stored in `<docs>/Sphere 2.0`
  instead of `<docs>/minisphere` as it was in prior versions.  SPK packages can
  be placed into the `games` subdirectory to have the startup game automatically
  pick them up.
* minisphere now looks for CommonJS modules in `~sgm/commonjs`.
* Enhances `Assert()` behavior.  If an assertion fails and the debugger is
  attached, choosing not to continue will cause a prompt breakpoint instead of
  throwing an error.  If the debugger is not attached, any failing assertions
  will be logged to `stderr` but otherwise ignored.
* Improves fullscreen behavior: Games are letter/pillarboxed to maintain their
  aspect ratio when switching into fullscreen mode.
* Screenshots are now given meaningful names based on the game filename and
  current date instead of random characters.
* The frame rate is now visible by default whenever a game is started using the
  `spherun` command, and has been moved to the lower right corner of the screen.
* When the debugger is attached, the engine now shows a small "SSJ" watermark in
  the lower left corner of the screen as a reminder.
* The engine now waits for sounds to stop playing before freeing them, even if
  the Sound object goes out of scope.  This allows a common Sphere idiom
  `new Sound("munch.wav").play()` to work as expected.
* With the debugger attached, you can now press F12 to pause game execution and
  turn over control to the attached debugger.  This can be useful when trying to
  debug glitches that don't lead to an exception.
* You can now change the minisphere Console verbosity level when developing in
  Sphere Studio by going to the Settings Center page.  V2 (high-level logging)
  is the default.
* Vastly improves object inspection in the Sphere Studio debugger.  Object
  contents will be displayed in a treeview, allowing you to drill down into
  properties, alleviating the need to scroll through a potentially huge JSON
  dump.
* The command to run minisphere Console from the command line has changed from
  `msphere` to `spherun`.  This will be the standard command to start a
  Sphere 2.0 engine in developer mode going forward.  The end-user engine has
  been renamed as well, to `minisphere`.
* `RNG.vary()` is now named `RNG.uniform()`.
* New API: `DebugPrint()`, for logging low-level debug information without
  cluttering the console.  `DebugPrint()` output is visible only with a debugger
  attached.
* New API: `DoEvents()`.  This function can be called in a long-running loop to
  avoid locking up the engine when you don't need to render anything or
  otherwise disturb the backbuffer.
* `Print()` now accepts multiple values, which are separated by spaces when
  printed.
* The `sphere` global object alias has been renamed to `global`, which is more
  obvious and matches Node.js.  Code relying on the `sphere` alias will need to
  be updated to work with minisphere 3.0.
* All minisphere API functions, constructors, and constants have been marked as
  non-enumerable, to avoid bloating the output when examining the global object
  in the debugger.  Everything is still fully writable and configurable, so as
  not to prevent monkey-patching.
* Fixes memory leaks in both Cell and minisphere, including a major one in
  Cell's packaging code which could have caused it to run out of memory during
  the installation step.
* minisphere will no longer fail to start if the underlying platform doesn't
  support shaders.  Instead, the Galileo `ShaderProgram` constructor will throw
  an error if called.  Shaders are always disabled when the engine is compiled
  against Allegro 5.0.


v2.1.6 - December 20, 2015
--------------------------

* Fixes a 2.1.5 regression which caused the last character to be cut off of the
  title bar text.

v2.1.5 - December 19, 2015
--------------------------

* The Sphere Studio plugin now includes a setting to test games in windowed
  mode.
* Fixes momentarily display of "Allegro" in the title bar instead of the game
  title during startup.
* Fixes minor audio corruption at the end of the stream when playing some WAV
  files.

v2.1.4 - December 16, 2015
--------------------------

* When an unhandled error is encountered, the error text can now be copied to
  the clipboard by pressing Ctrl+C.
* Fixes a bug where GrabImage and GrabSurface would grab a black screen when
  called from a map engine script, regardless of what was rendered that frame.

v2.1.3 - December 14, 2015
--------------------------

* Fixes a bug where evaluating an expression in the debugger would use the
  global object for 'this' instead of the active function's proper 'this'
  binding.

v2.1.2 - December 8, 2015
-------------------------

* Fixes a bug where JavaScript Error objects with messages of the form "not X"
  can be constructed with the wrong filename and line number information.
* Allegro version is now logged during startup.

v2.1.1 - November 25, 2015
--------------------------

* miniRT now reads initialization parameters from the game manifest. This means
  that, going forward, S2GM will be required to take full advantage of the
  library.
* The Sphere Studio plugin now adds the minisphere and Cell API references to
  the IDE's Help menu. This ensures the API reference will always match the
  installed engine version.
* Includes an updated version of Duktape with more descriptive JS error
  messages.


v2.1.0 - November 1, 2015
-------------------------

* Adds support for checked API extensions. By including a 'minimumPlatform'
  object in the game's S2GM manifest, S2GM-compliant engines can be made to
  check that the minimum API requirements are met before launching the game.
* Now has the ability to run JS scripts without a manifest, by executing, e.g.
  'msphere script.js' at the command line.
* Adds a new API function, SetScreenSize(), allowing games to change resolution
  at runtime.
* Improves JS error messages when attempting to read properties from
  non-objects (e.g. "cannot read property 'foo' of undefined", as opposed to
  "invalid base value").
* Running a script without a game() function will no longer produce an
  exception.
* Fixes a bug in KevFile:save() which, in certain circumstances, caused the file
  to be saved in a different location than it was opened from.


v2.0.1 - October 30, 2015
-------------------------

* Internal handling of file paths has been overhauled, fixing several crashes
  (including a segfault after returning from ExecuteGame) and making paths more
  predictable in general.
* Fixes segfaults and insane console output at higher log levels.
* Fixes a memory leak in the Async() trampoline due to a missing shutdown call.
* Adds an API reference for Cell to the distribution.

v2.0.0 - October 26, 2015
-------------------------

* Includes a new command-line compiler and packager for Sphere games, Cell!
  Using Cell, you can control the build process for your game using JavaScript
  and even compile your game to SPK format!
* More predictable path semantics: All Sphere 2.0 APIs dealing with filenames,
  including all constructors, are resolved relative to the game manifest.  The
  semantics of legacy APIs such as LoadImage() remain unchanged.
* New JSON-based .s2gm game manifest format allowing much more flexibility in
  authoring metadata for a game.  All data in the .s2gm is available to scripts
  by calling GetGameManifest().
* The minisphere redistributable now starts in full screen mode by default.
* Vastly improved Sphere Studio plugin with full support for Cell, including
  automated SPK packaging.
* Improved debugger allows evaluating local variables and expressions at any
  point in the call chain.
* Basic source map support.  When an error occurs and during debugging, if the
  game was compiled with Cell's '--debug' option, the path to the corresponding
  file from the source directory is displayed instead of its name within the
  game package.
* All APIs accepting ByteArrays will now also accept ArrayBuffer and TypedArray
  objects.  ByteArray is deprecated, but retained for backward compatibility.
* SoundStreams are no longer restricted to 8-bit samples.
* Audialis now supports 16- and 24-bit signed as well as 32-bit floating-point
  for both Mixer and SoundStream objects.
* New 'FileStream' object: A more robust and SphereFS compliant replacement for
  Sphere 1.x's RawFile, using ArrayBuffers instead of ByteArrays.
* The File object has been renamed to 'KevFile' to avoid confusion with the more
  general-purpose RawFile and FileStream objects.
* Now uses a custom build of Duktape to support 'const' in scripts.  As in
  Sphere 1.5, 'const' values are not actually constant and are in fact simply
  synonyms for an equivalent 'var' declaration.


v1.7.11 - September 9, 2015
---------------------------

* Re-fixes the eval bug originally fixed in 1.7.8 and accidentally reverted in
  1.7.10.
* Updates Duktape to allow objects with circular references to be displayed in
  the debugger.

v1.7.10 - September 8, 2015
---------------------------

* Upgrades Duktape to the latest build, which includes several ArrayBuffer
  fixes.
* Renames RNG.name() to RNG.string(). This is a breaking change, but makes the
  purpose of the function clearer.
* Improves RNG.string() performance by generating only one random number per
  character instead of 2.
* RNG.string() output can now include digits.

v1.7.9 - September 7, 2015
--------------------------

* Fixes a few bugs in debugger error intercept which caused Duktape to forcibly
  detach the debugger after issuing certain debug commands.

v1.7.8 - September 5, 2015
--------------------------

* TCP port 1208 is now used for debugging. The previous debug port was 812,
  which is in the well-known port range and can't be opened by a non-root user
  under Linux.
* Fixes a bug where an error thrown during a debug eval would cause a corrupt
  notification message to be sent, likely crashing the debugger.

v1.7.7 - September 1, 2015
--------------------------

* Fixes a bug where, when the debugger is attached, executing an infinitely
  recursive function would crash the engine instead of throwing a RangeError.
  Without the debugger attached, a double fault would be generated instead.

v1.7.6 - August 29, 2015
------------------------

* Sphere API functions are named in stack listings. This is helpful when
  debugging to see when, e.g. a MapEngine() call is on the stack.
* Upgrades Duktape to the latest build, improving performance.
* Debugging has been disabled for the Redistributable as it hurt performance
  even when not active.

v1.7.5 - August 22, 2015
------------------------

* The debugger will now automatically pause execution before a fatal exception
  is thrown to allow local variables and the callstack to be examined.

v1.7.4 - August 18, 2015
------------------------

* miniRT console output is now routed through Print() so it is visible to an
  attached debugger.

v1.7.3 - August 17, 2015
------------------------

* Assert() failures can now be ignored when a debugger is attached. Previously,
  nonfatal asserts were only enabled for minisphere Console.
* Reverts recent command line option renaming which made the engine incompatible
  with earlier versions. Options may change in 2.0, but for now it's best to
  retain compatibility here.
* miniRT now includes built-in console commands for BGM management:
    - bgm play <filename> [<fade>]
    - bgm push <filename> [<fade>]
    - bgm pop [<fade>]
    - bgm stop [<fade>]
    - bgm volume <0-1> [<fade>]

v1.7.2 - August 15, 2015
------------------------

* Debugging is now enabled only over loopback. Remote debugging isn't
  particularly useful without access to full matching source, and this avoids
  the need to add a firewall exception.
* While in Active debug mode (`--debug`), the engine will automatically
  exit if the debugger detaches cleanly.

v1.7.1 - August 12, 2015
------------------------

* Fixes a bug where a breakpoint placed inside of a conditional block
  will be triggered even if the condition isn't met.

v1.7.0 - August 11, 2015
------------------------

* The included analogue.js now uses EvaluateScript() to load map
  scripts; this ensures their filenames are associated with the code,
  allowing them to be debugged.
* Upgrades Duktape to the latest build, adding support for ArrayBuffer
  and TypedArray objects, and improving the CommonJS implementation
  (notably, 'module.exports' is now supported).
* The debug port (812) is now kept open, allowing debuggers to be
  attached and detached at will. This enables a debugger to easily
  reconnect if the connection is lost.
* The engine will now wait 30 seconds for the debugger to reconnect if
  the TCP connection is lost during a debug session.


v1.6.1 - July 31, 2015
----------------------

* Fixes listening sockets not taking connections from IPv4 clients.

v1.6.0 - July 27, 2015
----------------------

* minisphere can now act as a Duktape debug server by passing --debug on the
  command line. A debugger can attach remotely over TCP using Duktape's binary
  debug protocol.
* EvaluateScript() now returns the result of evaluation, as if the contents of
  the file were passed to eval().
* Vastly improved console logging.
* ByteArrays can now be resized at runtime using the new resize() method. This
  allows them to be used as dynamic buffers.
* Sockets can now be piped via the new pipe() and unpipe() IOSocket methods.
* Renames the global object to `sphere` for naming consistency with other
  JavaScript environments, where the global object is always lowercase (window,
  global, root...).


v1.5.3 - July 18, 2015
----------------------

* GetGameList() will now search, in addition to the engine directory,
  `~/Sphere Games` for usable games (game.sgm files and SPKs).

v1.5.2 - July 16, 2015
----------------------

* mini.BGM now supports crossfading.
* The Sound:volume accessor property now expects a floating-point value instead
  of an integer [0-255]. Sound:get/SetVolume() retain backwards compatibility.
* Log messages are now emitted preemptively, making it easier to find the source
  of an issue.
* More useful output when running at log levels 2 and 3.
* The command line is parsed much earlier in the initialization cycle.

v1.5.1 - July 14, 2015
----------------------

* Fixes an issue where attempting to create an 8-bit mixer will fail with
  a "cannot create hardware mixer" error.

v1.5.0 - July 12, 2015
----------------------

* NEW! Audialis API: Use Mixers to group related sounds and feed audio data
  directly to the audio system using the new SoundStream object.


v1.4.7 - July 5, 2015
---------------------

* SphereFS '~usr/' target directory changed from '<home>/Sphere Files' to
  '<home>/minisphere'.
* Fixes a bug where switching out of fullscreen mode can get the display stuck
  in limbo, not quite windowed but not quite fullscreen either. This prevented
  Alt+Tab from working whenever it happened, as well.
* Fixes a bug where the hitbox for scaled persons falls out of alignment with
  the sprite as it gets bigger.

v1.4.6 - July 2, 2015
---------------------

* Displays a default taskbar icon (the Spherical icon) when running a game
  with no icon.png provided.
* Scaling of taskbar icons (icon.png) has been improved to allow images
  larger than 32x32 with less distortion.
* Fixes a bug where screenshots taken with F12 would sometimes come out with
  washed-out colors.

v1.4.5 - June 29, 2015
----------------------

* Now logs game information (title, author, resolution) on startup.  Log
  level 2 or higher will also log the game path.

v1.4.4 - June 28, 2015
----------------------

* Fixes a bug where pressing F12 to take a screenshot won't create a
  Screenshots directory, and thus can't save the screenshot.
* miniConsole now responds accordingly to the Home and End keys.

v1.4.3 - June 27, 2015
----------------------

* Fixes a "not callable" error when attempting to call Socket:accept().

v1.4.2 - June 25, 2015
----------------------

* Fixes a bug where changes to a Sound's pitch, volume, etc. are lost between
  play() calls.
* Fixes a bug in Delay() causing it to hog the CPU.
* Fixes several Surface blend modes not working properly, notably the default
  'BLEND' mode.
* Fixes broken rendering of windowstyles with gradients.
* Fixes a bug where the engine would lock up if AreKeysLeft() is used in a loop
  to wait for a keypress.
* Fixes surface.setAlpha() not actually doing anything.
* Fixes surface.flipHorizontally/Vertically() not flipping the surface.
* Fixes surface.setPixel() unconditionally writing black pixels.
* Fixes MapToScreenX/Y() returning incorrect values for maps with parallax
  layers.
* Fixes a minor inaccuracy in person-person obstruction causing issues in games
  with tile-based movement.

v1.4.1 - June 22, 2015
----------------------

* Fixes a bug where top-level variables defined in a CoffeeScript script were
  not available from outside when using RequireScript().
* Fixes off-by-one rendering issues for a few graphics primitives, such as
  Line().

v1.4.0 - June 21, 2015
----------------------

* CoffeeScript support!
* The engine and all dependencies have been recompiled optimizing for speed.
  This slightly increases the size of the engine but greatly improves
  performance for CPU-bound tasks such as ColorMatrix application.


v1.3.2 - June 20, 2015
----------------------

* More informative error message when game() function doesn't exist, no more
  vague "not callable" errors!
* Zone scripts are now fired once immediately on the first step inside the zone,
  regardless of the step count set. This matches Sphere 1.5 behavior.
* Fixes Z-ordering glitches after looping around a repeating map.
* GetPersonX(), GetPersonY() are now properly normalized on repeating maps.

v1.3.1 - June 18, 2015
----------------------

* Fixes parallax camera sync issues, for reals this time!
* Adds a new API, GetActivePerson(), which can be called inside a talk or touch
  script to get the name of the person responsible for triggering the event.
* Fixes an engine crash when a person under the control of FollowPerson is
  destroyed in the middle of a diagonal movement.
* Changes target of the SphereFS ~usr/ prefix to <UserDocuments>/Sphere Files.
  This makes it easily accessible under Windows and ensures more sane placement
  when the engine is run under Wine.
* Fixes a regression introduced in 1.3.0 where player input is accepted even
  though the input person's command queue is not empty.
* Fixes a bug where zones loaded from a map file may randomly not fire.
* Fixes a bug where talk scripts won't run if commands were queued for the input
  person on the same frame the talk key is pressed.

v1.3.0 - June 17, 2015
----------------------

* SphereFS support! This provides new directory aliases (~usr/, ~sys/, ~sgm/)
  with more predictable semantics than the classic ~/ alias, which is of course
  maintained for backwards compatibility.
* Sphere Package format (.spk) support!
* 4-player support in map engine (AttachPlayerInput() et al.)
* Support for Sphere ColorMatrix objects.
* Support for zlib compression and decompression of ByteArrays.


v1.2.4 - June 11, 2015
----------------------

* Fixes mysterious "invalid base value" errors when a built-in constructor is
  shadowed by a global variable with the same name.

v1.2.3 - June 10, 2015
----------------------

* Fixes a bug where calling RawFile:close() may cause a crash when the RawFile
  object is later GC'd due a double fclose(). Affected Linux and possibly other
  platforms as well.
* Allows GetKey() to detect Ctrl, Alt, and Shift key presses.
* Fixes incorrect handling of tab characters during text rendering.
* Persons positioned on a hidden layer will no longer be hidden. This matches
  the behavior of Sphere 1.5.
* Fixes incorrect rendering of text containing non-ASCII characters. Note that
  some characters such as the euro sign (€) and trademark symbol (™) will be
  substituted with placeholders due to differences between the Unicode and
  Windows-1252 codepages.

v1.2.2 - June 8, 2015
---------------------

* Fixes map-defined BGM not playing--by implementing the feature. Nobody can say
  I'm not dedicated to compatibility!

v1.2.1 - June 7, 2015
---------------------

* Fixes a regression in GetCurrentZone() and GetCurrentTrigger() where an
  index is returned.
* Fixes out-of-sync scrolling issues with parallax layers on non-repeating maps.
* Fixes a bug in Font:wordWrapString() where text ending on a wrap boundary can
  cause the last word to be lost.

v1.2.0 - June 6, 2015
---------------------

* CommonJS module support!
* Globals are now accessible via the Sphere object. With this, global variables
  can be created from strict mode code, where it would normally be flagged as an
  error. This is similar to the 'window' object in a browser:
      Sphere.myVar = 812;
      Sphere.GetKey();
      // etc.
* New Zone and Trigger management APIs.
* Included kh2Bar can now be used as both a module and with a traditional
  RequireSystemScript() call.
* Font:clone() now works correctly.
* Fixes crashes under rare circumstances in Font:wordWrapString(). Also properly
  wraps long stretches of characters with no whitespace.
* Fixes a bug where replacing a glyph image fails to update the font metrics,
  causing rendering issues.
* More complete API documentation.


v1.1.8 - June 2, 2015
---------------------

* Fixes too-fast animation when persons are moved diagonally.
* Fixes a bug where queueing diagonal movement commands has no effect.

v1.1.7 - June 1, 2015
---------------------

* Fixes an occasional crash when calling ExecuteGame().

v1.1.6 - May 31, 2015
---------------------

* The third argument ('is_immediate') to the QueuePersonCommand() and
  QueuePersonScript() APIs is now optional. If not provided, the default is
  false.
* Fixes a startup crash when XInput is not found or fails to initialize.
* Fixes a startup crash on systems with no sound support.
* Fixes some extension strings having been coalesced due to a missing comma in
  the source. Seriously, who ever thought coalescing string literals was a good
  idea?

v1.1.5 - May 28, 2015
---------------------

* Improved zone and trigger handling. Triggers will now fire even if input is
  attached after the trigger is stepped on. This fixes triggers in Kefka's
  Revenge.
* Fixes a bug where the input person can be moved while following another
  person.
* Proper handling of degenerate rects in spritesets and zones, which caused
  collision checks to fail.
* Fixes a regression where calling IsKeyPressed() in a loop to wait for key
  release would lock up the engine (yes, again).
* Updates the map engine to use the new input routines, to take advantage of the
  sticky-keys fix in v1.1.3.

v1.1.4 - May 26, 2015
---------------------

* Cleanup to Script Error screen: Subdirectory information is now shown for
  script filenames and the error location is displayed on a separate line from
  the error text.
* Evaluating non-UTF-8 scripts with extended characters no longer causes the
  engine to fail.
* Font:wordWrapString() now correctly handles newlines.

v1.1.3 - May 25, 2015
---------------------

* GetToggleState() is implemented--finally.
* Keys should no longer get "stuck" when using Alt+Tab to switch away from the
  engine during gameplay.
* Filename of map is now shown when an error occurs in an embedded script.

v1.1.2 - May 23, 2015
---------------------

* GLSL shader support! Shaders are loaded from <game_dir>/shaders by default.
  To use:
    var shader = new ShaderProgram({ vertex: 'vs.glsl', fragment: 'fs.glsl');
    var group = new Group(shapeList, shader);
    // work with Group object as before

v1.1.1 - May 20, 2015
---------------------

* Fixes an issue where windowstyles with component images smaller than 16x16
  weren't tiled properly, due to an Allegro limitation.
* Full MNG animation support.  No FLIC though.  That's an ancient format anyway
  though, not really worth supporting.
* Fixes GradientCircle (finally!) and adds missing circle methods to Surfaces.
* New RNG.name() API: Generates a random string of letters which you can use
  internally to differentiate objects.

v1.1.0 - May 17, 2015
---------------------

This is an absolutely MASSIVE release.  Note: minisphere is now BSD-licensed.
This basically gives you absolute freedom to do with the engine and associated
source code what you wish, so long as you don't misrepresent yourself as the
author.

* Fixes a ton of bugs discovered since the release of minisphere 1.0.10,
  including a SetPersonScript() crash when passing JS functions.
* Now using Allegro 5.1.10.
* Native 64-bit engine included (engine64.exe).
* New "Console" builds of minisphere (console.exe and console64.exe) for
  Windows. This version of the engine displays a console window like a debug
  build (enabling minisphere's Print() function to work), but is fully
  optimized.
* Constructors and properties for all core Sphere objects:
      var sound = new Sound("munch.wav");
      sound.volume = 128;
      sound.play(false);
* Massive performance improvements, both to rendering as well as resource
  loading. Most assets load lightning fast!
* Key mappings for GetPlayerKey() can now be set programmatically via the new
  SetPlayerKey() API; these changes will be saved per-game and loaded along with
  that game's game.sgm.
* Assert(), a useful debugging tool. Throws an error if the asserted condition
  fails, a no-op otherwise. In the Console build, also gives you the option to
  ignore the failed assert and continue.
* Async(), queues a script to run on the next FlipScreen(). Similar to what
  setTimeout() does in a browser and required for, e.g. compliance with the
  Promises/A+ spec.
* TurboSphere-inspired "Galileo" graphics API, which allows scenes to be
  composed ahead of time using Shape and Group objects.
* TurboSphere-inspired Sockets API, with separate ListeningSocket and
  IOSocket objects. Sphere 1.5 sockets are still supported.
* Built-in MT19937-based random number generator, including a method
  (`RNG.normal`) to generate normally-distributed random values.
* Includes miniRT, a unified set of system scripts including a cooperative
  threading engine, cutscene engine, the Link query library, and a full-featured
  console, among other goodies. Simply add this to the top of your main script:
      RequireSystemScript('mini/miniRT.js');
* The engine now searches <game_dir>/scripts/lib for system scripts before
  looking in the system directory.
* API documentation is included in the default distribution. It may be somewhat
  incomplete, however...
* Playback position in Sounds can be set with microsecond precision.
* Overhauled FollowPerson() algorithm: Followers move under their own
  power and are affected by obstructions, making the overall effect much
  more pleasing.
* New API functions to manage persons with followers:
      GetPersonLeader()
      GetPersonFollowers()
      GetPersonFollowDistance()
      SetPersonFollowDistance()
* New path escape: `#~/path/to/file` specifies a path which is relative to the
  engine's system directory.


v1.0.10 - April 16, 2015
------------------------

* Experimental 64-bit engine (needs testing).
* Fixes IsKeyPressed() not recognizing modifier keys on right side of
  keyboard (#20).
* Improves SetPersonFrame compatibility (out-of-range frame is now
  wrapped instead of throwing an error).
* Fixes SetPersonFrame not resetting frame delay timer.
* Fixes a joystick assert error when running minisphere in OS X. (#19)
* Fixes wrong direction being rendered for 4-direction person sprites
  moving diagonally.
* Fixes random deadlocks when playing sounds.
* Adds some API functions from Sphere 1.6+.
