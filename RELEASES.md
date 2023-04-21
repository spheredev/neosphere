Release Notes
=============

neoSphere 5.9
-------------

* When targeting API level 4 or higher, functions like `FS.readFile()` that
  access the file system synchronously are now completely disabled.  Games
  targeting API 4+ should instead use the asynchronous `File` API, introduced
  in this release.  See the API documentation for usage.


neoSphere 5.8
-------------

* It is now possible to construct standard transformation matrices directly
  using static methods of the `Transform` class, without the need to call
  `new Transform()` first.  See the API documentation for more information.

* Support for the `global` binding is now disabled when targeting API level 4
  and higher.  Going forward, games should use the ECMA standard `globalThis`
  to access the global object.


neoSphere 5.7
-------------

* It is now possible to target Sphere v2 using only a `game.sgm` file.  To that
  end, Cell will now include the necessary fields in the SGM file when building
  a game targeting Sphere v2.  For full backward compatibility, a corresponding
  `game.json` will still be generated in the distribution.

* Save data (i.e. files written into `~/...`) is now stored in a `Sphere Saves`
  folder under the user's Documents folder.  If you have any save data stored
  in the old location (neoSphere -> Save Data under Documents), you'll need to
  move it to the new location if you want your games to continue to pick it up.

* `BufferStream` has been renamed back to `DataStream` for consistency with the
  naming of JavaScript's built-in `DataView`, which serves a similar purpose.
  The class is also better documented now.

* When targeting API level 4, `JSON.fromFile()` will no longer work.
  `FS.readFile()` should be used instead, with its new JSON read mode (refer to
  the API documentation).  For backward compatibility, `JSON.fromFile()` will
  continue to work in games targeting API level 3 and under.

* The `describe()` function has been re-added to the Cellscript API and should
  be preferred going forward, instead of assigning directly to `Sphere.Game`.

* When debugging a Sphere game in SSj, code originating inside one of the
  `sphere-runtime` modules is now skipped over.  This allows Step Into to be
  used more freely without the need to wade through system code.


neoSphere 5.6
-------------

* In preparation for `FileStream` access becoming asynchronous, `DataStream`
  has been removed from the Sphere Runtime API and its role replaced by the new
  `BufferStream` class. In general, code using `DataStream` to read structured
  data from binary files should instead call `FS.readFile()` with
  `DataType.Raw` and construct a `BufferStream` from the resulting data buffer.

* neoSphere now uses "neoSphere" for the name of its user data directory (in
  the user's Documents folder), instead of "miniSphere", to align with the
  engine rename.

* The legacy Sphere v1 "startup game" has been removed. When launched,
  neoSphere 5.6 and later will now show a standard file-select dialog instead
  so that the user can select a game from anywhere.


miniSphere 5.5
--------------

* `Texture.fromFile()` and other `.fromFile()` functions have been canonized.
  Calling the equivalent constructors in a game targeting API 3 will produce a
  deprecation warning in the engine's terminal output; these constructors will
  be completely disabled in API 4 in order to pave the way for full 
  cross-compatibility with Oozaru.

* miniSphere can now load TrueType fonts. To this end, `Font.fromFile()` will
  accept an additional parameter specifying the size of the font.

* When importing modules from folders, `package.json` handling has been updated
  to be more consistent with Node.js behavior. In particular, the engine now
  honors the `"type"` field, enabling it to load ES Module packages. 

* There is now experimental support for depth buffering when rendering to
  surfaces, including the backbuffer. The depth test can be customized by
  setting `Surface#depthOp` to the desired value. See the API documentation for
  more.

* It is now possible to import or `require()` modules installed using `npm`
  within Cellscript code.

* When importing Sphere Runtime modules, you can now refer to them as using a
  virtual path, e.g. `/lib/sphere-runtime.js`. This will improve your game's
  Oozaru compatibility, since browsers don't yet support import maps. Keep in
  mind, however, that this feature is experimental and may be subject to
  breaking changes in future releases.


miniSphere 5.4
--------------

* miniSphere and Cell now support the `.cjs` file extension introduced in
  Node.js v13.2.0.  Scripts with this extension are always loaded as CommonJS
  modules, regardless of whether `require()` or `import` is used.  This can be
  helpful if you need to load a CommonJS module using a relative path, since
  relative `require()` is not supported in ESM files.

* miniSphere 5.4 adds a new optional `strictImports` flag in the `development`
  section of `game.json`.  This flag, when enabled, causes SpheRun to enforce
  that your `import` specifiers are Oozaru-compatible--in particular, any
  imports referring to files in the project must include the file extension,
  such as `.js`.  Like other `development` flags, `strictImports` has no effect
  in production.

* `DirectoryStream` now includes additional `depth` and `extension` properties
  on result objects, making it easier to filter directory entries based on
  those criteria.  Keep in mind that since the new properties are part of the
  experimental API level 3, they will be disabled in retrograde mode.

* `Mouse#getEvent()` no longer returns `null` in case of an empty queue.
  Instead it now returns an object whose `.event` is set to `null` in this case
  to facilitate safe destructuring of the return value.  This might break code
  that specifically checked for a return value of `null`.  The old behavior is
  being treated as a bug and the change may be backported if necessary, so any
  code that does this should be updated as soon as possible.

* Several `Query` methods were renamed to align with the corresponding LINQ
  names.  This will inevitably break code that uses these methods; check the
  changelog for the full list.  Note that the Sphere Runtime API is not yet
  frozen.

* Cell now supports using a directory name for `Tool#stage()`.  Directories are
  considered to be permanantly out-of-date, so Cell will always run a tool
  which claims to build one.  This eases things for tools that dynamically
  determine their sources or create multiple output files, which would often be
  considered erroneously up-to-date because Cell didn't consider all the files
  involved.

* To improve sandboxing and ensure out-of-source builds always work correctly,
  starting with this version, Cell code can no longer write files and
  directories under `$/`.  This change might break builds that depend on `$/`
  being writable.

* Cell now tracks all files created using `FS.writeFile()` as build artifacts.
  These files will be therefore now be deleted when running `cell clean`, even
  if they have no corresponding `Target`.


miniSphere 5.3
--------------

* Several APIs were canonized in this release (refer to the API documentation
  to see which ones) and the maximum supported API level increased to 2.  If
  you put `apiLevel: 2` in your manifest, your game will require miniSphere 5.3
  or later.  If you still want to support earlier versions, specify
  `apiLevel: 1` in the manifest and test for newer APIs at runtime by checking
  the value of `Sphere.APILevel`.

* Cell's command-line syntax has been enhanced to support new commands.
  Notably, a few options have been promoted to full commands, e.g. `cell -c` is
  now `cell clean`.  If you need a quick primer on the new syntax, you can type
  `cell help` on the command-line.

* New `fromFile` factory methods have been added to all built-in classes whose
  constructor accepts a filename.  These methods return a promise for the
  new object, which allows assets to be loaded asynchronously in
  implementations where that is necessary.

* The `from` module has been rewritten for improved performance and along with
  several new query operators come some minor breaking changes.  `Query#skip`
  is now called `drop`, `Query#from` is now called `over` and the iteratee for
  `.first` and `.last` is now treated as a mapping function rather than a
  predicate.  As always, refer to the Sphere Runtime API documentation to get
  up to date on current API usage.

* `import` statements are now always treated as ES Module imports regardless of
  filename extension.  If your game or Cellscript needs to load CommonJS
  modules and is now doing so using `import`, you will need to switch to using
  `require()`.

* To improve end-user security and ensure the SPK package format is not a
  malware vector, it is no longer possible to relax or disable the SphereFS
  sandbox in production.  The `sandbox` manifest field has been moved into a
  special `development` namespace which will only honored by SpheRun when
  running a game from a local directory.  When using a relaxed or disabled
  sandbox during development, Cell will notify you when building for release
  that full sandboxing will be enforced in production.

* When executing a standalone `.js` or `.mjs` script from the command line,
  starting in miniSphere 5.3, you must use the `spherun` command.  Standalone
  scripts are executed with the SphereFS sandbox disabled, so allowing the
  end-user engine to run them was identified as a potential security issue.

* New `MouseKey` event codes were added (`MouseKey.Back`, `MouseKey.Forward`),
  and therefore games written for API 1 will now end up seeing key codes they
  can't match to a known constant.  This is a potentially breaking change if
  your game treats unrecognized event codes as an error.  Instead of producing
  an error, unknown event code should simply be ignored.

* `SSj.assert` has been added which lets you perform sanity checks during
  development without impacting performance in production.  Expensive checks
  can be wrapped in an arrow function which will only be called when running
  the game using SpheRun.

* The obsolete `assert` and `test` modules have been removed from the Sphere
  Runtime in preparation for an upcoming API freeze in miniSphere 6.0.  These
  modules were written based on old CommonJS specifications and don't really
  have a place in a modern JavaScript codebase.

* The undocumented scenelets (`fadeTo`, `tween`, `pause`, `call`) that were
  previously registered by default by the `scene` module have been removed.
  They had been retained for internal use, but the Scene API accidentally
  exposed them to user code.  If you're currently using one of the scenelets
  listed above, you will need to implement their functionality yourself.

* `DirectoryStream` objects can now be initialized in recursive mode.  In this
  mode, instead of listing subdirectories along with files, it will recursively
  descend into subdirectories to list the files in them.  This provides a
  convenient way to find files at any level of a directory structure using a
  single DirectoryStream.

* When calling `Transform#rotate` and specifying a vector to rotate about, the
  provided vector is now normalized.  This is a bug fix, but may be a breaking
  change if your game happened to rely on the previous behavior.  If your
  rotations seem broken after upgrading to mS 5.3, this is probably why.


miniSphere 5.2
--------------

* SpheRun now supports profiling!  Call `SSj.profile()` and pass it an object
  along with the name of a method and all subsequent calls to that method will
  be profiled.  Then simply run your game with `spherun --profile` and When the
  engine shuts down, a table showing the profiling results will be printed to
  the terminal!

* Using the new `Dispatch.onExit()` API, or `Thread#on_shutDown()`, your game
  can set up cleanup code which will run before the engine closes.  This makes
  it easier to implement autosave systems, for example, and can be used in both
  Sphere v1 and Sphere v2 codebases.

* Data compression is now supported as a first-class citizen of Sphere v2:
  `Z.deflate` and `Z.inflate` use the zlib DEFLATE compression algorithm, the
  same one historically used for Sphere v1's `DeflateByteArray`.  These APIs
  are also available for use in Cellscripts!

* You can now read and write PNG images in a Cellscript, and even work directly
  at the pixel level, using the new `Image` API.  Images are loaded as 32-bit
  RGBA to keep manipulation of individual pixels simple.

* `Surface` is now a proper subclass of `Texture`, allowing a surface to be
  used anywhere a texture is expected.  This opens the door for awesome
  render-to-texture effects and avoids the cost of making a copy incurred by
  calling `.toTexture`.

* Textures can now be manipulated directly at the pixel level using the new
  `Texture#download` and `Texture#upload` methods.  Keep in mind that these are
  both incredibly expensive so you probably don't want to call them every
  frame.

* `Shape.drawImmediate` has been added: this gives games the ability to draw
  vectorized primitives on-the-fly without the overhead of creating VertexList
  and Shape objects each time.

* The `image` module has been removed from the Sphere Runtime.  Games currently
  depending on the `Image` class should switch to using `Prim.blit` instead.


miniSphere 5.1
--------------

* Your game can now suspend and resume recurring Dispatch jobs using the new
  `JobToken#pause()` and `JobToken#resume()` APIs.  Suspended jobs keep the
  event loop alive but won't be executed again until they are resumed.

* `Thread` objects now have `.pause()` and `.resume()` methods as well.
  Pausing a thread suspends its update and input jobs, but not its render job.
  This allows you to pause updates for an entity while still allowing it to be
  visible on-screen.


miniSphere 5.0
--------------

* As of miniSphere 5.0.0, the Core API has been frozen.  Going forward, no
  further breaking changes will be made to the Core API and games written
  against it will continue to function as-is in future versions.  This freeze
  **does not** apply to the Sphere Runtime!

* miniSphere now uses ChakraCore for blazing-fast JavaScript performance.
  Chakra is the same engine used in Microsoft Edge and supports most modern
  JavaScript syntax natively, so you no longer need a `transpile()` step in
  your Cellscript to take advantage of ES2015+ features such as arrow
  functions, destructuring, even `await`!

* Thanks to the introduction of `async` and `await`, the event loop is now a
  first-class part of the Sphere development experience.  To that end, all
  Sphere v2 functions dedicated the old blocking style have been removed or
  refactored: both `Sphere.run()` and `screen.flip()` are gone, and
  `Sphere.sleep()` has been changed to return an awaitable promise instead of
  blocking the caller.

* mJS modules are now supported natively, without a transpiler.  This allows
  you to use `import` and `export` to organize your codebase into
  self-contained modules without the added complexity of CommonJS.  `require()`
  has in fact been deprecated and is now provided only for interop with
  transpilers such as Babel and modules originally written for Node.js.  New
  code should always use the ES2015 module syntax (`import`/`export`) and the
  `.mjs` file extension.

* The entire Sphere Runtime was overhauled and is now written entirely in mJS.
  This brought several breaking changes both major and minor.  The big changes
  are listed below, but be sure to review the API documentation to get fully
  up to speed!

* Because ES2015+ syntax is now supported natively, the Cell Runtime
  `transpile` module has been cut.  If you're currently using it in your build,
  you can simply change your Cellscript to use `install()` in place of
  `transpile()` and everything should work as before.

* `screen` was renamed to `Surface.Screen`, and the custom `screen` methods
  have been moved into the `Sphere` namespace.  So `screen.frameRate` becomes
  `Sphere.frameRate`, etc.

* `Dispatch.onUpdate()` and `Dispatch.onRender()` now take an options object
  as their second parameter.  Job priority is now be specified as a property of
  the options argument and defaults to 0.0 if not provided, as before.

* `DataReader` and `DataWriter` have been combined into a single class,
  `DataStream`, which inherits from `FileStream`.  This makes it easier to use
  as it's no longer necessary to construct a FileStream separately.  Naturally,
  any code using the old classes will need to be updated to work with the new
  class.

* The `Console` object has been refactored into a full-fledged class.  This
  allows an in-game console to be set up using explicit `new Console()` syntax
  rather than the somewhat awkward `Console.initialize()`.  It also makes it
  possible to create multiple consoles, in case that's ever needed.   Existing
  code using the `Console` object will need to be updated.

* The new `FocusTarget` class provides a centralized mechanism for managing
  input focus.  Only one focus target can have the focus at any given time,
  making this very well-suited to managing input focus in, e.g., menu systems
  and map engines.  Just import `FocusTarget` from `sphere-runtime` to try it!

* The `Pact` class has returned and provides a convenient way to make promises
  and settle them out-of-band without the awkwardness of working around the
  promise closure.  As long as you have a reference to both the Promise object
  and the Pact it came from, you can resolve or reject it at any time.

* `Scene#run()` now returns a promise that can be `await`ed and never blocks.
  The boolean parameter that specified whether or not to block until completion
  has been removed; if you want a scene to run in the background, simply ignore
  the promise.

* `Thread.join()` is no longer a blocking call and instead returns a promise
  that can be `await`ed.  This allows any thread to await termination of
  another without delaying other threads, regardless of how many joins are
  already underway.

* A new `sandbox` field in the JSON game manifest can be used to relax the
  SphereFS sandbox in order to ease development.  The default is a full sandbox
  as before; 'relaxed' allows use of absolute paths and write access to the
  game directory, while 'none' disables the sandbox completely.

* `Sphere.exit()` has been replaced with `Sphere.shutDown()`.  Unlike the
  former function, `Sphere.shutDown()` does not exit immediately but rather
  cancels all outstanding Dispatch jobs, allowing the event loop to unwind
  naturally on the next tick.


miniSphere 4.8
--------------

* The Core API includes a new class, `DirectoryStream`, which lets you game
  enumerate the contents of a directory, seek within the list, etc., much like
  you would read data from a file using `FileStream`.  DirectoryStream objects
  are also compatible with the ES6 iterator protocol so you can use them in
  `from()` query.  Refer to the API documentation for details.

* SphereFS now supports a new prefix, `$/`, which maps to the directory
  containing the startup script.  This should make importing local modules
  easier.

* `from()` can now enumerate live ES6 iterator objects (i.e. with a compatible
  `.next()` method), not just those with a `[Symbol.iterator]` method.

* The `Music` and `Logger` modules now have designated base directories and
  providing file extensions is optional.  See the API documentation for more
  details.

* A new `Image` class in the Sphere Runtime makes it easier to work directly
  with raster images, like you could in Sphere 1.x.  For now only basic
  blitting (like `Prim.blit()`) is supported; more features will be added going
  forward.

* The backbuffer is no longer cleared between frames during map engine
  processing, to improve compatibility with many Sphere 1.x games that relied
  on this bug.  There may be unintended fallout (graphical glitches) from this,
  so keep an eye out.


miniSphere 4.7
--------------

* The value of `Sphere.Platform` now includes the version number of the engine,
  while `Sphere.Version` has been redefined to return the version of the Sphere
  API, e.g. 2.  `Sphere.APIVersion` has been removed as it is now redundant.

* The `Shape` object has been completely overhauled to better support GPU-side
  vertex and index lists.  Rather than these lists being maintained internally,
  they are now exposed to JavaScript code via the new `.vertexList` and
  `.indexList` properties.  A few breaking changes were made to the API in
  order to support this; refer to the API documentation for more information.

* Games can now assign a `Transform` object to a surface's `transform` property
  to change the projection matrix.  To create projection matrices, there are
  two new `Transform` methods: `.project2D()` for orthographic projection, and
  `.project3D()` for perspective projection.

* Clipping is now supported for all surfaces, not just the screen.  Your game
  can now call `.clipTo()` on any Sphere v2 surface object to set its clipping
  box, just like you can do with `screen`.

* `Socket` objects can now be reused by simply calling the new `.connectTo()`
  method to reestablish a connection.  This even works for already-connected
  sockets; the existing connection will be closed in that case.

* For convenience, `FS.readFile()` and `FS.writeFile()` now work directly with
  strings instead of buffer objects.  This makes them unsuitable for use with
  binary data due to the UTF-8 processing involved.  Going forward, games that
  must work with binary files should use a `FileStream` object instead.

* miniSphere will no longer run on computers without a shader-capable GPU.  If
  you must continue supporting such hardware, you'll have to stick to v4.5.11
  or earlier.


miniSphere 4.6
--------------

* When using standard JavaScript modules (`.mjs`), the main module of a
  Sphere v2 game can now optionally use `export default` to export a class.
  The exported class should implement, at the very least, a `start()` method
  to be called by the engine on startup.  If the startup class derives from
  `Thread`, you need only implement the `on_update()` and `on_render()`
  handlers and miniSphere will kick off your main thread for you!

* Several Sphere Runtime modules have been removed or renamed.  Pact is no
  longer available, and term has been renamed to Console.  All CommonJS modules
  making up the Sphere Runtime now also match the object they export.  For
  instance, `require('scene')` returns the standard `Scene` object.

* Most `Prim` methods have been renamed to make them more self-documenting and
  to make it more obvious that these represent immediate-mode drawing.  For
  example, `Prim.line()` is now called `Prim.drawLine()`.  Refer to the API
  documentation for the full list.

* The Sphere v2 Core API now includes a `Sample` class which allows a single
  loaded sound to be played multiple times simultaneously, on any mixer.  This
  is great for sound effects.

* `describe()` has been removed from the Cellscript API and Cellscripts must
  now manipulate the contents of `Sphere.Game` directly.  If desired, you can
  use ES6 `Object.assign()` to get semantics similar to `describe()`.

* `SSJ.log()` and `SSJ.trace()` have returned and allow logging text to the
  attached debugger.  `SSJ.log()` output will also be visible in the terminal,
  while `SSJ.trace()` output will not.

* You must now call `Console.initialize()` to enable the debug console for your
  game.  Loading the console module with `require()` will no longer enable it
  automatically.


miniSphere 4.5
--------------

* Handling of save data has been changed.  In order for a game to save, a
  `saveID` field must now be present in `game.json`, which determines the
  location the engine will use for that game's save data.  If no `saveID` is
  defined, any SphereFS path beginning with `~/` will be rejected with a
  ReferenceError.

* `Image` has been renamed to `Texture` in service to eventual API convergence
  with Oozaru.  The Web platform already exposes an incompatible `Image` class,
  so this change was unavoidable.

* `system` has been renamed to `Sphere` to better reflect its role as a
  namespace for low-level engine services.  Additionally, properties such as
  `.version` and `.apiLevel` have been capitalized, since they are runtime
  constants.

* `Sphere.sleep()` has reverted to its original behavior; it no longer runs a
  frame loop and therefore now takes a time in seconds rather than frames.

* `FS.openFile()` has been refactored into a `FileStream` constructor.  The
  second argument of the constructor uses easy-to-read constants such as
  `FileOp.Read` as opposed to the arcane C `fopen()` mode strings used with
  `FS.openFile()`.

* Games can now get or set the fullscreen mode programmatically by using the
  `screen.fullScreen` property.



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
