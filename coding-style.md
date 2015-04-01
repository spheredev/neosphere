minisphere Coding Conventions and Etiquette
===========================================

You don't *have* to follow these guidelines, but if you don't, do note
that I suffer from OCD and will almost assuredly edit your code
mercilessly afterwards! :o) 

Indentation
-----------
Use tabs for indentation, not spaces. OCD aside, I'm not one of those
coders who obsesses over the "correct" number of spaces to use for
indentation. Best to use tabs and let everyone choose their own
preferred indent size.

Variable Declarations
---------------------
Declare all variables at the top of a function. No exceptions. Okay,
well, there is ONE exception: If you need to pass an impromptu array or
struct to a function (as with, e.g. `al_draw_prim()`), you can declare
it inline, as filling it in afterwards would be ugly.

Casts
-----
Keep casts to a minimum, and try not to add casts for the sole purpose
of shutting up an implicit-conversion warning. A clean compile is nice,
but not at the expense of making the code harder to read. There's a
reason minisphere is coded in C and not C++!

`const` Correctness
-------------------
Try to maintain const correctness at all times, and keep in mind that
it is almost always the wrong solution to cast away constness. If a
function returns a const pointer, it's const for a reason--fail to
respect that and things may just blow up in your face!

Pull Requests
-------------
Forking minisphere and opening pull requests on GitHub is allowed and
encouraged; however, as mentioned at the top of this document, be
prepared to have your code edited for style before the changes are
merged in.

Commit Etiquette
----------------
* Commit titles should be written in the imperative ("Fix the hunger-pig
bug") and, if possible, descriptions in the simple present tense ("Fixes
a bug where a hunger-pig eats you in 2 seconds". The idea is, if someone
has a copy of the repository and applies a commit, the commit message
should describe what will happen to their copy. Past-tense commits (e.g.
"Fixed X bug") suggest retroactivity and are thus more confusing to work
with. Avoid them.

* Avoid creating commits with only non-functional changes. Too many of
these make bisection (using `git bisect`) harder as it adds extra noise
to the process. Again, I will edit pull requests for style before
merging them, so extra commits to fix whitespace, etc. are entirely
redundant.
