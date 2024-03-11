neoSphere Changelog
===================

v5.9.3 - TBD
------------

* Fixes a bug where passing an `options` object to `Sample#play()` doesn't work
  properly and may throw an exception.

v5.9.2 - December 22, 2023
--------------------------

* Fixes a bug that causes `DirectoryStream` to report directories as files when
  running from an SPK package.
* Fixes a bug where `Query` doesn't pass a key to certain callbacks while
  querying object properties.
* Fixes a bug that causes `Query#last()` to return the first match instead.
* Fixes a bug that causes `Query#single()` to incorrectly return `undefined`.

v5.9.1 - December 3, 2023
-------------------------

* Updates the copyright date to 2024.

v5.9.0 - May 30, 2023
---------------------

* Adds new experimental asynchronous `File` API for games targeting API 4+.
* Adds a new predefined color, `Color.CosmicLatte`.
* Renames `Color.EatyPig` to `Color.EatyPink`.
* Removes support for calling `FS.readFile`, etc. in games targeting API 4.


v5.8.2 - January 27, 2023
-------------------------

* Changes the SSj badge to say "SSj CLI" instead of just "SSj", to make it
  clearer that a command-line debugger is attached.

v5.8.1 - August 12, 2022
------------------------

* Removes the architecture (x86/x64) from the command-line header.

v5.8.0 - March 23, 2022
-----------------------

* Adds a new API, `Color.fromRGBA()`, for constructing colors from 8-bit RGBA
  component values.
* Adds a new API, `Transform.Identity`, for quickly getting an identity matrix.
* Adds new static methods to `Transform` to construct basic transformations
  without the need to call `new Transform()` first.
* Adds a new predefined color, `Color.EatyPig`.
* Fixes a bug that caused the dimensions of `Transform#matrix` to be swapped.
* Removes support for `global` when targeting API 4 or higher.


v5.7.2 - January 25, 2022
-------------------------

* Adds `RT.Version` for getting the current API revision of the Sphere Runtime.
* Renames `Thread` to `Task`, retaining the former as a temporary alias.
* Removes the `.ready` and `.whenReady()` APIs added in the previous release.
* Removes support for `new Surface(fileName)` and `Surface.fromFile()` in games
  targeting API 4 or higher.
* Fixes an issue with the `cell init` template that left new projects with an
  invalid `describe()` call in their Cellscripts.
* Fixes a bug that caused the engine to stop responding to debugger commands
  while the JavaScript error screen was being displayed.

v5.7.1 - January 4, 2022
------------------------

* Adds new `.ready` and `.whenReady()` APIs which allow games to check if an
  asset (texture, sound, etc.) is completely loaded before using it.
* Changes the handling of the first parameter of Cell's `install()` to be
  relative to `@/` by default, instead of `$/`.

v5.7.0 - December 15, 2021
--------------------------

* Adds support for targeting Sphere v2 without a JSON manifest.
* Adds back support for the `describe()` function in Cellscripts.
* Adds support for showing a game's targeted API level in SSj.
* Adds the ability to automatically step over Sphere Runtime code while
  debugging.
* Adds a new JSON read mode for `FS.readFile()`.
* Changes the Documents folder hierarchy so both screenshots and save data are
  put into a single `Sphere Saves` directory.
* Removes support for the undocumented `JSON.fromFile()` function when
  targeting Sphere v2 API level 4 or higher.
* Fixes an issue where neoSphere annoyingly creates an empty directory for
  Sphere v1 games in the user's Documents folder on every startup.
* Fixes a bug where Cell rejects save IDs containing spaces as invalid.
* Fixes a bug where the engine may crash on startup if it's unable to launch
  the requested game.


v5.6.4 - November 16, 2021
--------------------------

* Changes SSj's `--no-pause` option to `--pause` and makes not pausing on
  startup the default behavior.
* Changes the label color for warnings in the terminal from red to yellow.
* Removes all Sphere Studio components from the neoSphere codebase.

v5.6.3 - August 15, 2021
------------------------

* Fixes a bug where creating a new project with Sphere Studio would fail due to
  changes to the project template.

v5.6.2 - July 21, 2021
----------------------

* Adds support for targeting the future Sphere API level (currently API 4).
* Fixes a bug where `cell init` produces a Cellscript that targets API level 4
  instead of the current stable API 3.

v5.6.1 - July 6, 2021
---------------------

* Removes support for 32-bit versions of Windows.
* Fixes a bug where Cell would show a deprecation warning even when targeting
  API level 3.

v5.6.0 - May 10, 2021
---------------------

* Adds support for multiple texture units and, by extension, additional
  `sampler` uniforms in shaders.
* Adds a new class, `BufferStream`, to the Sphere Runtime.
* Changes the name of the engine from "miniSphere" to "neoSphere".
* Renames the user data directory from "miniSphere" to "neoSphere".
* Removes the `DataStream` class from the Sphere Runtime.
* Removes the Sphere v1 "startup game" in favor of showing a standard
  file-select dialog when launching neoSphere.
* Fixes a bug where `Query#shuffle()` doesn't shuffle the final result.
* Fixes a bug where `Mouse#position` returns an array with the coordinates in
  the wrong order.


v5.5.2 - November 30, 2020
--------------------------

* Adds a prompt for the new game's screen resolution when running `cell init`.
* Improves compatibility of the Makefile with macOS.
* Changes the `Music` functions in the Sphere Runtime to load audio files
  asynchronously and return promises if applicable.
* Fixes a bug where trying to load a non-font file or unsupported font format
  using `new Font()` or `Font.fromFile()` could cause the engine to segfault.
* Fixes a bug where `cell init` could sometimes append garbage bytes to the end
  of the generated Cellscript.
* Fixes a bug where the depth and blend modes weren't reset before showing the
  exception screen, sometimes making the error message unreadable.
* Fixes a bug where SSj doesn't remove cleared breakpoints from the breakpoint
  list, often leading to a segfault in either miniSphere or SSj as their
  respective breakpoint lists become out-of-sync.
* Fixes a bug where breakpoints set in Sphere Studio keep being hit even after
  clearing them in the IDE.
* Fixes a memory leak that can occur after setting array uniforms for a shader
  which subsequently goes unused.

v5.5.1 - September 13, 2020
---------------------------

* Adds a warning when calling constructors such as `new Texture()` with a
  filename in games targeting API level 3.
* Changes the default `.depthOp` for surfaces to `DepthOp.AlwaysPass`.
* Fixes a bug where async read and write weren't supported in retrograde mode.
* Fixes a bug where `DepthOp.Equal` is treated like `DepthOp.AlwaysPass`.

v5.5.0 - August 12, 2020
------------------------

* Adds support for loading TrueType fonts using the `Font` class.
* Adds support for Node.js-like automatic module type detection for `.js`
  files.
* Adds support for importing npm modules in Cell code.
* Adds support for seeing the types of variables while debugging with SSj.
* Adds depth buffer support for surfaces.
* Adds a new API, `Surface#clear()`, for clearing the entire contents of a
  surface including its depth buffer.
* Adds support for Oozaru-compatible `/lib/foo.js` specifier format for loading
  Sphere Runtime modules.
* Canonizes the `.fromFile()` APIs, as well as a few other functions,
  increasing the API level to 3.
* Fixes a bug where `Key.Tilde` isn't recognized on macOS.
* Fixes a bug where trying to load something other than an RFN file using the
  font API can cause the engine to segfault.


v5.4.2 - February 20, 2020
--------------------------

* Adds experimental `Thread#suspend()` API, which works like `Thread#pause()`
  but also pauses rendering.
* Adds support for mp3 playback through the `Sound` API on Windows.
* Renames `FileStream.open()` to `FileStream.fromFile()`.
* Renames `Socket.connectTo()` to `Socket.for()`.
* Fixes an issue where `from()` didn't check its argument, leading to a
  nonsensical stack trace later if the caller accidentally passes `null`
  or `undefined`.

v5.4.1 - January 5, 2020
------------------------

* Changes `strictImports` to also disable CommonJS module support.
* Removes the `xml` module from the Sphere Runtime.
* Fixes a bug where Cell writes the wrong values for `name`, `author`, and
  `description` in `game.sgm`.

v5.4.0 - December 1, 2019
-------------------------

* Adds several `Query` methods: `apply`, `call`, `concat`, `elementAt`, `join`,
  `memoize`, `pull`, `single`, `skipLast`, `skipWhile`, `takeLast`,
  `takeWhile`, and `zip`.
* Adds `Mouse#position` which returns a tuple `[x, y]` for easy destructuring.
* Adds mouse-button activation support to the `console` module.
* Adds a new `FS.match()` API for matching filenames and paths against "glob"
  patterns such as `**/*.js`, useful for filtering `DirectoryStream` output.
* Adds new `depth` and `extension` properties to `DirectoryStream` output to
  make filtering even easier.
* Adds support for using a directory as the output of `Tool#stage()` in Cell.
* Adds support for the `.cjs` file extension to facilitate smoother interop
  between ESM and CommonJS code.
* Adds a `strictImports` manifest flag to enforce Oozaru-compatible imports.
* Improves sandboxing by disabling write access to `$/` in Cell code.
* Improves Cell build hygiene by tracking files created with `FS.writeFile()`
  as build artifacts and cleaning them as necessary.
* Renames `Query#drop()` to `Query#skip()` for consistency with LINQ.
* Renames `Query#reduce()` to `Query#aggregate()`.
* Renames `Query#over()` to `Query#selectMany()`.
* Renames `Query#uniq()` to `Query#distinct()`.
* Fixes a bug where destructuring the return value of `Mouse#getEvent()` can
  throw an error due to being `null`.
* Fixes a bug where `Query#any()` and `Query#all()` can return `undefined` in
  certain cases instead of a boolean value.
* Fixes a bug where `Query#count()` can return `null` instead of `0`.
* Fixes a bug where `require()` can't be used to load system modules like
  `sphere-runtime`.
* Fixes a bug where Cell fails to properly normalize the pathname passed to
  `Tool#stage()`, potentially leading to undesirable behavior later.


v5.3.0 - December 25, 2018
--------------------------

* Adds support for games targeting API level 2, canonizing several Core APIs.
* Adds `cell init` to initialize a new Sphere source tree on the command line.
* Adds asynchronous asset loading functions, e.g. `Sound.fromFile()`, to
  improve cross-compatibility with Oozaru.  Refer to the miniSphere 5.3 release
  notes for further information.
* Adds a pop-up message when pressing F12 to confirm a screenshot was taken.
* Adds `Joystick.P1` through `Joystick.P4`, providing built-in default gamepad
  inputs for up to four players.
* Adds new async methods to `Socket` for performing connnections and I/O via
  the event loop, allowing a game to `await` the arrival of data and avoiding
  the need to explicitly check the state of the socket every frame.
* Adds `SSj.assert` to automatically verify assumptions made while coding.
* Adds `Socket#noDelay` which disables Nagle's algorithm for a connection.
* Adds `Server#numPending`, the length of a Server's connection backlog.
* Adds `JSON.fromFile()` for loading and parsing a JSON file in a single call.
* Adds `Font#widthOf()` and `Font#heightOf()` convenience calls.
* Adds a new parameter to `FS.readFile` to control how file contents are
  returned.
* Adds a new class to the `from` module, `Query`, which lets you prefabricate
  queries and run them as many times as you want on any source.
* Adds a new Sphere Runtime module, `tween`, for handling simple animations.
* Adds support for `-h` as an alias for `--help` for all command-line tools.
* Adds `FS.directoryOf()`, `FS.extensionOf()` and `FS.fileNameOf()` for taking
  apart path strings.
* Adds a new `development` manifest field to control behavior under SpheRun.
* Adds API support for the Back and Forward buttons found on modern mice.
* Adds a new option, `recursive`, for DirectoryStream, to include files in
  subdirectories.
* Adds `apiVersion` and `apiLevel` to the example Cellscript.
* Improves module handling; all `import` statements regardless of extension are
  now loaded as ESM code.
* Improves security by forcing full SphereFS sandbox enforcement in production.
* Improves security by disabling execution of bare scripts using `minisphere`.
* Improves `BlendOp` by making it into a class, allowing games to define their
  own custom blending modes.
* Improves performance of code using `SSj` namespace methods in production by
  avoiding unnecessary native calls.
* Improves performance of `from()` queries across the board.
* Improves bare-script execution by loading `.js` files passed on the command
  line as module code, allowing them to use `import`.
* Improves the performance of `Transform#matrix`, particularly on first-access.
* Improves first-access performance of `Sphere.Game` by avoiding an unnecessary
  JSON round-trip conversion.
* Improves Cell's command-line syntax.  Many options have been replaced with
  easy-to-remember commands like `cell build` or `cell pack`.
* Removes the `assert`, `delegate`, `joypad`, and `test` modules.
* Removes several internal-use-only Scenario scenelets (`fadeTo`, `call`,
  `playSound`, `tween`) which sometimes clashed with game code wanting to use
  those names.
* Fixes a crash when calling `MapEngine`/`FlipScreen` from the main function.
* Fixes an issue where the cursor is hidden while sitting over the game window.
* Fixes an issue where `FS.fullPath` doesn't add a trailing slash when passed
  the path of a known directory.
* Fixes an issue where `Socket#close` disposes of the underlying socket object,
  preventing it from being reused.
* Fixes a bug where the `MULTIPLY` blend mode is rendered the same as `ADD`.
* Fixes a bug where Cell puts `dist/` in the PWD by default instead of in the
  directory of the project being built.
* Fixes a bug where calling `Sound#play` without passing a Mixer doesn't work.
* Fixes a bug where the vector passed to `Transform#rotate` is not normalized
  before calculating the rotation matrix, leading to unpredictable behavior.


v5.2.13 - September 6, 2018
---------------------------

* Adds `Sphere.main` which provides access to the object created by the engine
  when the main module exports a class.
* Changes the default `Console` prompt from `$` to `>`.
* Changes `Thread#start()` to an async function which returns only after the
  Thread's startup handler has run to completion.
* Fixes a bug where `Sphere.sleep()` prevents the engine from shutting down
  until the sleep period expires.
* Fixes a bug in which `Scene#call()` doesn't await `async` functions.
* Fixes a bug where an `async` function used for a recurring Dispatch job can
  be re-entered before an earlier invocation has completed.

v5.2.12 - August 17, 2018
-------------------------

* Improves the Core API documentation by adding minimum API levels for each
  function and property documented.
* Fixes a crash bug where calling certain Sphere v1 map engine functions
  without the map engine running causes a null pointer dereference and ensuing
  segfault.
* Fixes a bug where cancelling the last recurring job using `JobToken#cancel`
  leaves the event loop spinning indefinitely, forcing the user to close the
  game window manually.
* Fixes a bug where passing a Symbol to `SSj.log()` throws an exception.
* Fixes a bug where `Mouse#getEvent()` returns the wrong cursor coordinates
  when the game window is scaled or in fullscreen mode.
* Fixes a bug where `Mouse#getEvent()` produces way too many click events.

v5.2.11 - July 1, 2018
----------------------

* Updates ChakraCore to version 1.10.0.
* Fixes an issue where miniSphere crashes with a segfault when a `.js` script
  attempts to dynamically import a module that doesn't exist.

v5.2.10 - June 18, 2018
-----------------------

* Fixes a bug where passing insanely large values to `SetLayerSize` could cause
  an exploitable buffer overflow.
* Fixes a bug which sometimes causes miniSphere to be unable to load JPEG
  images.

v5.2.9 - June 5, 2018
---------------------

* Fixes a bug that can cause the engine to segfault upon calling `import()` or
  `require()` in the presence of circular module dependencies.
* Fixes a bug where `null` is treated as an empty object in the debugger.
* Fixes bugs which cause `Infinity`, `NaN` and `undefined` values to be
  rendered as strings in the debugger.

v5.2.8 - May 24, 2018
---------------------

* Fixes a regression which prevented using most of the experimental API.
* Fixes a few errors in the Sphere Studio new project template which prevented
  it from running without changes.

v5.2.7 - May 20, 2018
---------------------

* Improves JavaScript performance by enabling idle processing in ChakraCore.
* Fixes an issue where errors for failed mJS module imports don't include the
  filename of the importing module.
* Fixes a bug where Cell doesn't show the offending script filename when a
  build fails due to a JavaScript syntax error.

v5.2.6 - May 14, 2018
---------------------

* Fixes a bug where specifying a nonexistent module ID for `import`
  sometimes produces a nondescript SyntaxError instead of the proper
  error message.

v5.2.5 - May 12, 2018
---------------------

* Fixes a bug in which the engine refuses to run a bare `.js` or `.mjs`
  file citing an API level mismatch.

v5.2.4 - May 11, 2018
---------------------

* Adds back support for `index.mjs` when running at API level 1.
* Changes the API level check on startup to only accept games targeting the
  current stable API level, to avoid a forward compatibility issue.
* Changes the maximum advertised API level back to 1.
* Changes the API level of `Color.PurwaBlue` and `Color.RebeccaPurple` to 2.
* Removes the Cell warning for `index.mjs` when targeting API level 1.

v5.2.3 - May 4, 2018
--------------------

* Adds a warning when a Cell build produces any output files named `index.mjs`
  and targets API level 1.
* Changes the API level to 3 of all APIs introduced in miniSphere 5.2.
* Fixes an issue where SPK packages built using Cell 5.2.0 through 5.2.2 won't
  run under miniSphere 5.1.

v5.2.2 - May 1, 2018
--------------------

* Fixes a regression in 5.2.0 which causes miniSphere to crash or otherwise
  misbehave when run with no command-line arguments.
* Fixes a segfault when stopping a sound at verbosity level 3 or higher.

v5.2.1 - April 29, 2018
-----------------------

* Adds a warning to Sphere v1 Cell builds for which the main script has a
  `.mjs` extension, since it will not run as module code.
* Adds a warning to Cell builds targeting the still-experimental API L2.
* Fixes `apiLevel` in a Sphere v1 build causing an error rather than a warning.
* Fixes a bug that causes miniSphere to crash on startup when a game's main
  script file doesn't exist.

v5.2.0 - April 27, 2018
-----------------------

* Adds support for passing command-line arguments to a game's `main()`.
* Adds profiling support to SpheRun; call `SSj.profile()` and pass an object
  and method name to have all subsequent calls to that function timed and
  included in a detailed Performance Report on shutdown.
* Adds `Image` to the Cellscript API for manipulating PNG images at build time.
* Adds an `apiLevel` field to the JSON manifest format which allows you to
  specify the minimum Sphere v2 API level supported by your game.
* Adds a command-line option, `--retro`, used to emulate older API levels.
* Adds new APIs for data compression, accessible through the `Z` namespace and
  available to both Sphere games and Cellscripts.
* Adds `Shape.drawImmediate()` function which avoids the overhead of creating
  VertexList and Shape objects for immediate-mode drawing.
* Adds an experimental `Surface#blendOp` property for setting the blending mode
  for objects drawn to a surface, à la Sphere v1 surfaces.
* Adds `Dispatch.onExit()`, which lets you run code after the event loop exits.
* Adds `Texture#upload()` and `Texture#download()` to allow direct manipulation
  of a texture's RGBA bitmap.
* Adds `Thread#on_shutDown()` for running code when a thread terminates.
* Adds `Pact#toPromise()` for cases where you don't want to allow outside code
  to prematurely resolve a pact.
* Adds `print()` as an alias for `SSj.log()`.
* Adds `SSj.now()` for getting microsecond-accurate timestamps under SpheRun.
* Adds the ability to use `Surface` objects as textures, without the need to
  call `.toTexture()` first.
* Adds long-overdue support for all Sphere 1.x Surface blend modes.
* Adds `[Symbol.toStringTag]` and `.constructor` for Sphere v2 objects.
* Improves font handling so that the default font can be loaded from an SPK
  package, avoiding the need to distribute the physical `system/` directory.
* Improves the overall performance of all API functions and constructors.
* Improves performance for all `Prim` drawing functions.
* Renames the `--performance` command-line option to `--profile` to reflect its
  new purpose.
* Removes the pointless `--no-sleep` command-line option.
* Removes the Sphere Runtime `Image` class from the API.
* Fixes an internal memory leak mostly affecting `Dispatch` job execution.
* Fixes a bug in which calling `FlipScreen()`, `DoEvents()`, or `MapEngine()`
  after entering the event loop leads to a segmentation fault.
* Fixes a bug in which calling `DoEvents()` doesn't run promise reaction jobs.
* Fixes a bug where Sphere v1 `Font#drawTextBox` output isn't properly clipped.
* Fixes a bug where calling Sphere v1 exit functions or closing the game window
  can prevent promise reactions from running.
* Fixes a bug where the Sphere v1 `Font#setCharacterImage()` API fails to
  recalculate the font metrics after changing the image, causing text to be
  rendered incorrectly afterwards.
* Fixes a bug where repeating a `list` command in SSj could show some of the
  same lines again.


v5.1.3 - February 18, 2018
--------------------------

* Adds support for `Promise#finally()` from ES2018.
* Improves the fatal error screen.  The error message is displayed on a red
  background both for greater emphasis and to make it more immediately obvious
  that something went wrong.
* Improves error messages.  The first letter of all error messages is now
  capitalized for consistency and several messages have been clarified to make
  them easier to understand.
* Fixes an issue where uncaught exceptions in async functions aren't picked up
  by SSj.
* Fixes an issue where using an `async` function as the main entry point
  (`export default` from main module) will cause the game to crash on startup
  with a TypeError.
* Fixes a bug which causes `Music.adjustVolume()` to throw a ReferenceError.
* Fixes a bug where `Dispatch.now()` jobs and promise continuations can run
  before the update phase instead of after it.

v5.1.2 - February 7, 2018
-------------------------

* Adds a new `logFileName` option for `new Console()` allowing you to specify
  where the log file will be saved.
* Adds exception handling for promise-based (async) code: uncaught errors
  in `async` functions will now cause a game to crash, rather than being
  silently ignored.
* Cell will now package the entire `#/` directory when making an SPK package.
* Logging is now disabled by default for `Console` objects.
* Fixes a bug where code in `.then()` continuations or async functions can end
  up running after a runtime error occurs, leading to strange behavior or even
  a hard crash.

v5.1.1 - January 23, 2018
-------------------------

* Fixes an issue where the engine can crash on startup when loading a game with
  circular module dependencies.

v5.1.0 - December 25, 2017
--------------------------

* Adds `JobToken#pause()` and `JobToken#resume()` methods to allow games to
  pause and resume Dispatch jobs at any time.
* Adds `Thread#pause()` and `Thread#resume()` methods.
* Adds `index.mjs` to the list of filenames recognized by the module loader.
* Adds a new predefined color, `Color.StankyBean`.
* Optimizes Surface and Texture size properties: `.width` and `.height` are now
  cached as own properties of the object after the first access.
* Fixes a bug where `Sphere.restart()` causes some things to be rendered in
  software afterwards, leading to massive slowdown.
* Fixes a bug in `XML.readFile()` which made the function completely unusable.
* Fixes a bug where `SSj.log()` logs "undefined" when passed an Error object
  with no stack trace.
* Fixes the Sphere Studio template after fallout from the `Surface.Screen`
  rename.


v5.0.1 - November 2, 2017
-------------------------

* Improves `SSj.log()` and `SSj.trace()` output for error and function objects.
* Fixes a bug where calling `Exit()` won't shut down the game if the built-in
  map engine is running.
* Fixes a bug where the source code of the mJS module shim isn't shown in SSj.
* Fixes a bug where calling `Dispatch.cancelAll()` can stop promise resolution.
* Fixes a bug where calling `FocusTarget#yield()` on an out-of-focus target can
  allow it to mysteriously regain focus later.

v5.0.0 - October 31, 2017
-------------------------

* miniSphere and Cell now use the ChakraCore JS engine under the hood, vastly
  improving JavaScript execution performance and bringing long-overdue native
  support for ES2015+ syntax and built-ins to Sphere.
* Overhauls the entire Sphere Runtime to take full advantage of the event loop
  as well as modern JavaScript features such as classes, `async` functions, and
  promises.
* Adds native support for ES2015+ syntax and mJS modules without transpilation.
* Adds back the `Pact` class, a more intuitive way to manage promises.
* Adds a new `FocusTarget` class as a centralized means to manage input focus.
* Adds a new `DataStream` class, which extends from `FileStream` to allow more
  easily reading binary data such as integers and strings from a file.
* Adds `Sphere.shutDown()` which initiates an asynchronous exit.
* Adds a new `inBackground` option for Dispatch jobs, for setting up background
  tasks that shouldn't keep the event loop alive by themselves.
* Adds an optional `sandbox` field to `game.json` which can be set to either
  `relaxed` or `none` to relax the file system sandbox and ease development.
* Adds `SSj.flipScreen()`, useful for debugging rendering code.
* Adds a `[Symbol.iterator]` to `DirectoryStream`, allowing directory contents
  to be enumerated using a standard `for...of` loop.
* Adds a new `--performance` command line option for SpheRun which disables the
  stepping debugger to ensure JavaScript code runs at full speed.
* Adds `fullScreen` manifest field to specify a game's default fullscreen mode.
* Adds support for quick refs to SSj: when using `examine`, this assigns a
  numeric handle to each object in the result, which you can quickly drill into
  by typing, e.g., `x *123`.
* Adds `Sphere.Compiler` which evaluates to the name and version number of the
  compiler used to build the current game.
* Changes `Sphere.sleep()` to return a promise instead of blocking, allowing it
  to be used with `await` so as to avoid starving the event loop.
* Changes `Console` into a full-fledged class, which allows for a familiar
  `new Console()` usage and gives games the ability to set up multiple consoles
  if desired.
* Changes `RNG` to be compatible with the ES2015 iterator protocol.
* Changes `SSj` namespace functions to be no-ops in the redistributable engine.
* Changes `SSj.log()` to perform JSON serialization if its argument is an
  object.
* Changes SSj commands `eval`, `examine` to not require quoting the expression.
* Renames `screen` to `Surface.Screen` and moves the custom `screen` properties
  into the `Sphere` namespace.
* Renames `from.Array()` and `from.Object()` to lowercase to match typical
  JavaScript naming conventions.
* Renames `screen.frameRate` and `screen.now()` to `Sphere.frameRate` and
  `Sphere.now()`, respectively.
* Renames `Dispatch.cancel()` to `JobToken#cancel()`.
* Renames `Color#fade()` to `Color#fadeTo()`.
* Renames `Keyboard#getChar()` to `Keyboard#charOf()`.
* Renames `fragment` and `vertex` Shader constructor options to `fragmentFile`
  and `vertexFile`, respectively.
* Removes the experimental `Person` class from the Sphere Runtime.
* Removes the now-redundant `DataReader` and `DataWriter` classes.
* Removes `screen.flip()`, `Sphere.run()` and `Sphere.exit()` in favor of the
  engine-provided event loop.
* Removes the Cell `transpile` module in favor of promoting native ES2015.
* Improves the startup routines to also look in `dist/` for a bundled game.
* Improves the SSj debugging experience by preventing the engine from switching
  to fullscreen mode as long as the debugger is attached.
* Improves internal handling of UTF-8 strings, fixing several bugs related to
  text encoding.  Notably, `FS.readFile()` now correctly handles the UTF-8
  signature/BOM if one is present.
* Improves error reporting.  SpheRun now prints a complete JavaScript backtrace
  to the terminal when a JavaScript runtime error occurs.


v4.8.8 - September 21, 2017
---------------------------

* Adds `FS.evaluateScript()`, used for loading Sphere v1 and browser scripts
  without having to fall back on the Sphere v1 API.
* Adds `Image#width` and `Image#height` properties.
* Adds `SoundStream#length` and removes `SoundStream#bufferSize`.

v4.8.7 - September 16, 2017
---------------------------

* Fixes a bug where paths beginning with `~/` were incorrectly interpreted as
  referring to the save data directory in Sphere v1 code, leading to a runtime
  error.

v4.8.6 - September 12, 2017
---------------------------

* Fixes a bug where the backbuffer texture is freed prematurely, causing the
  engine to crash on shutdown.
* Fixes a bug where miniSphere can crash or behave strangely if it's unable to
  determine the desktop resolution on startup.
* Fixes a bug where miniSphere can crash after calling `ExecuteGame()` if the
  engine fails to reinitialize.

v4.8.5 - September 10, 2017
---------------------------

* Improves Sphere 1.x backward compatibility by aligning small maps to the
  top-left of the screen instead of centering them, especially important for
  games using a render script.
* Improves the window scaling algorithm so that 320x240 games again run at 2x
  on 1366x768 displays.

v4.8.4 - August 22, 2017
------------------------

* Improves Cell game manifest generation to produce an error if the file named
  by `Sphere.Game.main` doesn't exist at the end of a build.
* Fixes an issue where running "Package Game" in Sphere Studio would build the
  game in `$/.staging` instead of the project's usual build directory.
* Fixes an issue in which a shape isn't allowed to have a vertex buffer smaller
  than its index buffer.
* Fixes a bug in Console which prevented the command line cursor from flashing.
* Fixes a bug where throwing either `null` or `undefined` causes a segfault.
* Fixes a bug where `Transform#project3D()` will accept negative or zero values
  for `near` and `far` parameters.
* Fixes a bug where the `dataReader` and `dataWriter` modules can throw an
  exception, caused by a forgotten import.

v4.8.3 - August 19, 2017
------------------------

* Adds a new `FS.relativePath()` API for computing partial file and directory
  paths for debugging and display purposes.
* Changes `Transform#rotate()` to use degrees instead of radians.
* Improves game manifest validation by disallowing SphereFS prefixes other than
  `@/` in the startup script path.
* Improves the organization of the Sphere Runtime to avoid duplicating modules
  shared between miniSphere and Cell.
* Fixes a bug where accessing Surface#transform can cause a segfault.
* Fixes a bug where changes to a surface's projection matrix are sometimes
  ignored.
* Fixes a bug where the screen's current projection matrix is used to display
  the fatal exception screen.
* Fixes a bug where calling `Shape#draw()` with no arguments causes a segfault.
* Fixes some bugs in the Sphere v1 implementation where a nonsensical relative
  pathname can be returned from certain API functions, e.g. `GetCurrentMap()`.

v4.8.2 - August 16, 2017
------------------------

* Fixes a bug in the internal audio streaming logic which corrupted the output
  of SoundStreams with more than one channel and made them sound choppy.
* Fixes a bug where legacy `Spriteset#filename` includes a SphereFS prefix.
* Fixes a bug where Cell will write an invalid script path to `game.sgm` if the
  script filename includes the `@/` prefix.

v4.8.1 - August 14, 2017
------------------------

* Adds `DirectoryStream#dispose()` to the API, for closing an open directory.
* Renames `SoundStream#buffer()` to `SoundStream#write()`.

v4.8.0 - August 12, 2017
------------------------

* Adds a new `DirectoryStream` built-in class which allows you to enumerate the
  contents of a directory.  Works in both miniSphere and Cell!
* Adds a new SphereFS prefix, `$/`, which resolves to the directory containing
  the startup script for miniSphere, or the root of the source tree for Cell.
* Adds a new `Image` class to the Sphere Runtime which allows working directly
  with images, like in Sphere v1 code.
* Adds `FS.directoryExists()` for checking whether a directory exists.
* Adds support for enumerating live iterators in `from.iterable()`.
* Adds a new experimental Person class to the Sphere Runtime which makes it
  easier to work with map engine persons.
* Improves windowed mode scaling behavior when miniSphere is run on a HiDPI/4K
  display.
* Improves the Music and Logger modules by giving them default base directories
  (like in Sphere 1.x) and making the file extension optional.
* Improves Sphere 1.x compatibility by avoiding clearing the backbuffer between
  frames during a `MapEngine()` call.
* Improves the maintainability of the codebase with extensive refactoring.  Any
  bugs, let me know!
* Renames the `analogue.js` system script to `persist.js` to improve
  compatibility with Sphere 1.x.
* Fixes several issues in the implementation of `FS.rename()` that could cause
  the function to behave destructively on some platforms.
* Fixes a bug where Cell `FS.writeFile()` can accept a buffer object as input.
* Fixes a bug in Cell `FS.fileExists()` where it returns true for directories.
* Fixes a bug in which a path specified uplevel from a SphereFS prefix, e.g.
  `@/../filename`, is not correctly detected as a sandbox violation.


v4.7.2 - July 28, 2017
----------------------

* Adds a new `cell-runtime` master module for use in ES6 Cellscripts, as an
  analogue to `sphere-runtime`.
* Renames `FS.resolve()` to `FS.fullPath()` and adds relative path support.
* Renames `FS.exists()` to `FS.fileExists()` since the function doesn't work
  for directories.
* Renames `FileStream#size` to `FileStream#fileSize`.
* Fixes some inconsistencies between modules in the Cell Runtime and their
  Sphere Runtime equivalents.
* Fixes an issue where chaining `Dispatch.now()` causes the engine to get stuck
  in an infinite loop of processing newly added jobs.
* Fixes a bug where `require()` doesn't canonicalize module filenames properly,
  causing SSj Blue to sometimes not be able to find the source file.
* Fixes a bug where building a large project in Sphere Studio can hang Cell.
* Fixes a bug where `SoundStream` streams are always created in mono mode.
* Fixes a bug where `DataReader` and `DataWriter` can't be imported by either
  `require()` or `import`.
* Fixes a bug where constructors can be called without using `new`.
* Fixes a bug where stepping over a `Dispatch.now()` with SSj sometimes locks
  up the engine and prevents it from responding to further debugger commands.

v4.7.1 - July 24, 2017
----------------------

* Fixes a long-standing bug where getting caught in an infinite loop prevents
  the engine from responding to debugger commands.
* Fixes a bug where delays, including those from normal frame rate limiting,
  cause actively playing SoundStreams to starve, leading to stuttering.
* Fixes a bug where reading `Shape#texture` from a shape with no assigned
  texture is not handled properly and may cause the engine to crash.

v4.7.0 - July 20, 2017
----------------------

* Adds support for creating 2D (orthographic) and 3D (perspective) projection
  matrices.
* Adds support for setting a clipping rectangle when rendering to a `Surface`.
* Adds new `VertexList` and `IndexList` objects, used to store vertices and
  indices, respectively, directly on the GPU for fast access at render time.
* Adds a new `.transform` property to `Surface` which allows changing the
  surface's projection matrix.
* Adds new methods to `Model` for setting shader uniforms: `.setBoolean()`,
  `.setFloatArray()`, and `.setIntArray()`.
* Adds `Socket#connectTo()` which allows the same socket object to be used for
  multiple sessions.
* Adds the engine version number to the string returned by `Sphere.Platform`.
* Adds support for the Sphere v1 `CreateSpriteset()` and `Spriteset#save()`
  functions.
* Changes the signature of the `Shape` constructor in order to support vertex
  and index lists and make it easier to use.
* Changes `FS.readFile()` and `FS.writeFile()` to work directly with strings
  instead of ArrayBuffers.
* Removes support for GPUs without shader support.  If the engine can't create
  a shader-capable display, it will now simply fail to start rather than trying
  to fall back on the fixed-function pipeline.
* Renames SSJ to SSj, with a lowercase J, to make the name more recognizable.
* Renames `Sphere.APIVersion` to `Sphere.Version`.
* Fixes several limitations in the internal handling of spritesets which were
  preventing full Sphere 1.x compatibility.
* Fixes several bugs in internal Dispatch job management, including one that
  can cause the engine to crash with a segfault.
* Fixes a bug where setting the clipping box out of bounds has no effect.
* Fixes excessive OpenGL state changes in the internal rendering logic, vastly
  improving performance for games using the Sphere v2 graphics API.


v4.6.0 - July 4, 2017
---------------------

* When using ES6 modules (`.mjs`), the main module can now `export default` a
  class and miniSphere will automatically instantiate it and call its `start()`
  method.  This mirrors how Sphere v1 engines call the global `game()` function
  on startup.
* Adds a new `Sample` class which works like `Sound` but loads the sound into
  memory instead of streaming it and allows multiple instances to be played at
  the same time.
* Adds methods to `Model` for setting vector-valued shader uniforms.
* Adds `Transform#matrix`, allowing direct access to the individual matrix
  cells of a transformation.
* Adds back the `SSJ` object, to allow sending text to the attached debugger
  (and optionally, to standard output).
* Adds `Prim.blitSection()`, to allow drawing only part of an image.
* Adds `Console.initialize()` which must now be called manually to enable the
  debug console.  Simply doing `require('console')` is no longer enough.
* Adds a new module, `sphere-runtime`, which brings together all the standard
  modules and allows multiple symbols to be imported simultaneously by using
  `import {}`.
* Adds support for `Sphere.Game` in Cellscripts, allowing the data in the JSON
  manifest to be manipulated directly.
* Adds more missing Sphere v1 API functions, further improving compatibility.
* Renames several standard modules, e.g. `term` becomes `console`.
* Renames `Prim` functions to better reflect their immediate nature and improve
  code readability, at the expense of some extra verbosity.  For example,
  `Prim.line()` becomes `Prim.drawLine()`.
* Renames `defScenelet()` to `Scene.defineAction()`.
* Removes the `console` object from the Core API, as it turned out to be easily
  confused with the standard console module.
* Removes the `pact` module from the standard library.
* Removes the `minify` module from Cell's standard library.
* Removes `describe()` from the Cellscript API.
* Updates the internal graphics framework to use a pixel-perfect backbuffer,
  which improves screenshots and fixes several rendering-related bugs.
* Updates the Sphere Studio project template with code to illustrate the use of
  ES6 class syntax as well as better showcasing the standard library.
* Fixes a crash that happens when using `SetTileImage()` or `SetTileSurface()`.
* Fixes a bug where an error thrown during execution of a Console command will
  crash the game with a JavaScript error.
* Fixes a bug where Cell will sometimes not detect a filename conflict.
* Fixes a bug where `Dispatch.onRender()` jobs are processed in reverse order
  following a call to `Dispatch.onUpdate()`, and vice versa.
* Fixes a bug where miniSphere calls the render script before the update script
  on map engine startup, leading to compatibility issues.
* Fixes a bug in `LineSeries()` and `PointSeries()` that made them always throw
  a "two or more vertices required" error.
* Fixes a bug where Sphere v1 `Surface#cloneSection()` and `Surface#rotate()`
  could introduce graphical artifacts on some platforms.
* Fixes a bug where Sphere v2 `Sound#pan` range is treated as [-255,+255]
  instead of the documented range of [-1.0,+1.0].


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
