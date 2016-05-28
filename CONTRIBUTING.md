minisphere Coding Conventions and Etiquette
===========================================

You don't *have* to follow these guidelines, but if you don't, do note that I
suffer from OCD and will almost assuredly edit your code mercilessly afterwards!
:o)

Use of Monster Drinks
---------------------

Make sure you drink a bunch of Monster drinks before writing any minisphere
code, especially when implementing entirely new features.  It really works!

**note:** Red Bull *doesn't* work.  At all.

Naming Convention
-----------------

Names of variables, functions, etc. should use the typical C naming convention
of all lowercase with words separated by underscores, e.g. `big_fat_pig`.
Global variables should be prefixed with `g_` and static variables (including
those at file scope) with `s_`. There is one exception: simple utility functions
with no side effects can leave out the word separator when appropriate, e.g.
`nativecolor()`.

Placement of Braces
-------------------

Functions, structs and enums get their opening and closing braces on separate
lines:

```c
const char*
path_filename_cstr(const path_t* path)
{
    return path->filename;
}
```

Control statements such as `if`, `switch`, and `for` should have their opening
brace on the same line as the statement, and any `else if` and `else` clauses
should start on the line after the closing brace of the clause above.

There is one exception to the above: Any control statement with a long,
multi-line condition gets its opening brace on a new line:

```c
if ((al_key_down(&keyboard, ALLEGRO_KEY_LCTRL) || al_key_down(&keyboard, ALLEGRO_KEY_RCTRL))
    && al_key_down(&keyboard, ALLEGRO_KEY_C))
{
    is_copied = true;
    al_set_clipboard_text(g_display, msg);
}
```

Function Conventions
--------------------

Function definitions in minisphere code look like this, with the storage class
and return type on a separate line from the function name and parameters:

```c
static bool
drink_a_bunch_of_monster_drinks(int how_many)
{
    if (how_many > 812)
        return false;

    return true;
}
```

Function prototypes are always on a single line, regardless of the number of
parameters (which are named), and function names and parameter lists should be
lined up:

```c
int  feed_pig     (const char* what);
void feed_cow     (int num_cats, bool eat_pig);
bool feed_gorilla (bool eat_cow, bool eat_universe);
```

Don't try to line up individual parameters, it would be a mess.

Variable Declarations
---------------------

* Declare all variables at the top of a function.  No exceptions.  Okay, well,
  there is ONE exception: If you need to pass an impromptu array or struct to a
  function (as with, e.g. `al_draw_prim()`), you can declare it inline, as
  filling it in afterwards would be uglier.

* Variable declarations should have the first characters in their names lined
  up.  Don't try to line up the initializers, however, as this increases the
  temptation to add an initialization clause to every single variable--which is
  unnecessary and complicates debugging since it can mask logic bugs.

Multi-line Expressions
----------------------

Splitting overlong expressions across multiple lines is encouraged.  When
continuing an expression on the next line, always try to start the continuation
line with an operator.  That goes double for multi-line conditionals:

```c
script_type = s_cam_y < 0 ? MAP_SCRIPT_ON_LEAVE_NORTH
    : s_cam_x >= map_w ? MAP_SCRIPT_ON_LEAVE_EAST
    : s_cam_y >= map_h ? MAP_SCRIPT_ON_LEAVE_SOUTH
    : s_cam_x < 0 ? MAP_SCRIPT_ON_LEAVE_WEST
    : MAP_SCRIPT_MAX;
```

Indentation
-----------

Use tabs for indentation, not spaces.  OCD notwithstanding, I don't generally
obsess over the "proper" number of spaces to use for indentation.  Better to use
tabs and let everyone choose their own preferred indent size.

Use of Typecasts
----------------

Keep typecasting to a minimum.  If the compiler warns you about storing the
value of a wider type into a narrower one (`int64_t` -> `int`, for instance), it
might be worth rethinking your approach.  Don't blindly add casts to shut the
compiler up; not only does that make the code more difficult to read, but it can
easily hide bugs.

Const Correctness
-----------------

Try to maintain const correctness at all times.  It's nearly always the wrong
solution to cast away constness: If a function returns a const pointer, the
pointer is const for a reason.  Fail to respect that and things may just blow up
in your face!

Pull Requests
-------------

Forking minisphere on GitHub and opening pull requests is allowed and very much
encouraged.  Of course, as mentioned at the top of this document, your code may
be edited for style before the changes are merged.

Commit Etiquette
----------------

* Commit titles should be written in the imperative ("Fix the hunger-pig bug")
  and, if possible, descriptions in the simple present tense ("Fixes a bug where
  a hunger-pig eats you in 2 seconds"). If you have a copy of the repository and
  apply a commit, the commit message should describe what will happen to your
  copy. Note that I'm more worried about the title than the body: Past-tense
  commit titles (e.g. "Fixed X bug") suggest retroactivity and are thus more
  confusing to work with. Avoid them.

* Avoid logging commits with only non-functional changes. Too many of these make
  bisection (using `git bisect`) harder as it adds extra noise to the process.
  Again, I will edit pull requests for style before merging them, so extra
  commits to fix whitespace, etc. are entirely redundant.
