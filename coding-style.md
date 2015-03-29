minisphere Coding Conventions
=============================

You don't HAVE to follow these guidelines, but if you don't, do note
that I have OCD and will almost assuredly edit your code mercilessly
afterwards! :o) 

Indentation
-----------
Use tabs for indentation, not spaces. OCD aside, I'm not one of those
coders who obsesses over the 'correct' number of spaces to use for
indentation. Best to use tabs and let everyone choose their own
preferred indent size.

Variable Declarations
---------------------
Declare all variables at the top of a function. No exceptions. Okay,
well, there is ONE exception: If you need to pass an impromptu array to
function (as with al_draw_prim, for example), you can declare it inline,
as filling it in afterwards would be ugly.

Casts
-----
Keep casts to a minimum, and try not to add casts for the sole purpose
of shutting up an implicit-conversion warning. A clean compile is nice,
but not at the expense of making the code harder to read. There's a
reason minisphere is coded in C and not C++!

Pull Requests
-------------
Opening pull requests on GitHub for minisphere is allowed and
encouraged; however, as mentioned at the top of this document, be
prepared to have your code edited for style before the changes are
merged in.
