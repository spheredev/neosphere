minisphere Changelog
====================

v3.3.0 - TBD
------------

* Adds a new `Color.mix()` API call to perform color mixing (either
  proportional or weighted).  `BlendColors()` and `BlendColorsWeighted()` are
  retained for backward compatibility.
* Adds a complete set of predefined colors based on the X11 color set, provided
  as static properties of the `Color` object.  For example, `Color.White`,
  `Color.DodgerBlue`, `Color.Chartreuse`, etc.
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
