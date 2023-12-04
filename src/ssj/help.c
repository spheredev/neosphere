/**
 *  Sphere: the JavaScript game platform
 *  Copyright (c) 2015-2024, Fat Cerberus
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither the name of Spherical nor the names of its contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
**/

#include "ssj.h"
#include "help.h"

void
help_print(const char* command_name)
{
	if (command_name == NULL || strcmp(command_name, "") == 0) {
		printf(
			"Short (1-2 character) names are listed first, followed by the full name of the \n"
			"command. Truncated full names are allowed as long as they are unambiguous.     \n\n"

			" bt, backtrace    Show a list of all function calls currently on the stack     \n"
			" bp, breakpoint   Set a breakpoint at file:line (e.g. scripts/eaty-pig.js:812) \n"
			" cb, clear        Clear the breakpoint with a given breakpoint index           \n"
			" c,  continue     Run either until a breakpoint is hit or an error is thrown   \n"
			" d,  down         Move down the call stack (inwards) from the selected frame   \n"
			" e,  eval         Evaluate a JavaScript expression                             \n"
			" x,  examine      Show all properties of an object and their attributes        \n"
			" f,  frame        Select the stack frame used for, e.g. 'eval' and 'var'       \n"
			" l,  list         Show source text around the line of code being debugged      \n"
			" s,  stepover     Run the next line of code                                    \n"
			" si, stepin       Run the next line of code, stepping into functions           \n"
			" so, stepout      Run until the current function call returns                  \n"
			" v,  vars         List local variables and their values in the active frame    \n"
			" u,  up           Move up the call stack (outwards) from the selected frame    \n"
			" w,  where        Show the filename and line number of the next line of code   \n"
			" q,  quit         Detach and terminate your SSj debugging session              \n"
			" h,  help         Get help with SSj commands                                   \n\n"

			"Type 'help <command>' for help with individual commands.                       \n"
		);
	}
	else if (strcmp(command_name, "backtrace") == 0) {
		printf(
			"View the contents of the callstack. This allows you to see which function calls\n"
			"are in progress and is especially useful in tracing your game's execution path \n"
			"leading up to an error.                                                        \n\n"
			"SYNTAX:                                                                        \n"
			"    backtrace                                                                  \n"
		);
	}
	else if (strcmp(command_name, "breakpoint") == 0) {
		printf(
			"Set a breakpoint at <file:line>.  When you have an idea in which part of the   \n"
			"program a bug resides but aren't sure of the exact cause, breakpoints can be   \n"
			"invaluable.  When the breakpoint is reached, execution will pause, leaving SSj \n"
			"in control.                                                                    \n\n"
			"The ID number of the set breakpoint will be given, which will be needed if you \n"
			"want to clear it later using 'clear'.                                          \n\n"
			"SYNTAX:                                                                        \n"
			"    breakpoint             - to list active breakpoints                        \n"
			"    breakpoint <file:line> - to set a breakpoint                               \n"
		);
	}
	else if (strcmp(command_name, "clear") == 0) {
		printf(
			"Clear the breakpoint <index> established using 'breakpoint'.  When you no      \n"
			"longer need a breakpoint, you should clear to prevent execution from pausing in\n"
			"that spot again.                                                               \n\n"
			"To list active breakpoints and their indices, simply enter 'breakpoint' with no\n"
			"arguments.                                                                     \n\n"
			"SYNTAX:                                                                        \n"
			"    clear <index>                                                              \n"
		);
	}
	else if (strcmp(command_name, "continue") == 0) {
		printf(
			"Resume normal execution.  If a breakpoint is hit or an error is thrown which is\n"
			"not caught, SSj will pause execution again.                                    \n\n"
			"SYNTAX:                                                                        \n"
			"    continue                                                                   \n"
		);
	}
	else if (strcmp(command_name, "down") == 0) {
		printf(
			"Move down the callstack relative to the selected frame, towards the innermost  \n"
			"call.  A number can be provided which specifies the number of frames to move.  \n"
			"If no argument is given, 'down' moves down by one (1) frame.                   \n\n"
			"SYNTAX:                                                                        \n"
			"    down         - move down by one (1) frame                                  \n"
			"    down <steps> - move down by <steps> frames                                 \n"
			);
	}
	else if (strcmp(command_name, "eval") == 0) {
		printf(
			"Evaluate a JavaScript expression <expr>. This works exactly like JavaScript    \n"
			"eval() and allows you to examine objects, assign to variables, or anything else\n"
			"you can do with a line of code.                                                \n\n"
			"The expression is evaluated using the lexical environment of the selected      \n"
			"stack frame, and if the result is an object, only enumerable properties will be\n"
			"displayed.                                                                     \n\n"
			"SYNTAX:                                                                        \n"
			"    eval <expr>                                                                \n"
		);
	}
	else if (strcmp(command_name, "examine") == 0) {
		printf(
			"Evaluate a JavaScript expression <expr>.  If the result is a primitive value   \n"
			"such as a string or number, 'examine` works exactly like `eval'.  If the result\n"
			"is an object, all of its properties (including non-enumerable), their          \n"
			"attributes, and their values are listed in long form.                          \n\n"
			"Each object in the output includes a numeric handle preceded by an asterisk,   \n"
			"e.g. '*123'.  These are called 'quick refs', and they let you drill down into  \n"
			"nested objects by using `examine` on the handle.  For instance, `x *123`.      \n\n"
			"SHORT NAME: x                                                                  \n"
			"SYNTAX:                                                                        \n"
			"    examine <quick-ref>                                                        \n"
			"    examine <expr>                                                             \n"
		);
	}
	else if (strcmp(command_name, "frame") == 0) {
		printf(
			"Select a callstack frame to examine.  Use 'backtrace' to see the full list of  \n"
			"frames on the callstack.  When SSj pauses execution, the innermost JavaScript  \n"
			"frame is selected, which corresponds to the function call in progress.         \n\n"
			"Sometimes it's necessary when investigating a bug to view the state earlier in \n"
			"the call chain.  The 'frame' command changes the context used for the following\n"
			"commands:                                                                      \n\n"
			"    - eval                                                                     \n"
			"    - examine                                                                  \n"
			"    - list                                                                     \n"
			"    - vars                                                                     \n\n"
			"The program counter isn't affected: 'continue' and stepping commands will      \n"
			"resume in the innermost call regardless of the stack frame selected.           \n\n"
			"SYNTAX:                                                                        \n"
			"    frame <index>                                                              \n"
		);
	}
	else if (strcmp(command_name, "list") == 0) {
		printf(
			"Print a few lines of source code surrounding the line currently being executed.\n"
			"By default, 10 lines of source are shown.  If you provide a number with the    \n"
			"'list' command, that number of lines will be printed.                          \n\n"
			"'list' may also be used to show the text of any source file in the project so  \n"
			"long as you know its SphereFS filename.                                        \n\n"
			"SYNTAX:                                                                        \n"
			"    list                     - list 10 LOC around the line being executed      \n"
			"    list <lines>             - list <lines> LOC around the line being executed \n"
			"    list <lines> <file:line> - list <lines> LOC around <file:line>             \n"
		);
	}
	else if (strcmp(command_name, "stepin") == 0) {
		printf(
			"Execute the next line of source code.  If a function is called, execution will \n"
			"pause at the start of that function.  This lets you trace into function calls. \n\n"
			"SYNTAX:                                                                        \n"
			"    stepin                                                                     \n"
		);
	}
	else if (strcmp(command_name, "stepout") == 0) {
		printf(
			"Continue execution until the active function call returns.  Execution will     \n"
			"pause again at the call site.                                                  \n\n"
			"SYNTAX:                                                                        \n"
			"    stepout                                                                    \n"
			);
	}
	else if (strcmp(command_name, "stepover") == 0) {
		printf(
			"Execute the next line of source code, including all function calls in their    \n"
			"entirety.                                                                      \n\n"
			"SYNTAX:                                                                        \n"
			"    stepover                                                                   \n"
		);
	}
	else if (strcmp(command_name, "up") == 0) {
		printf(
			"Move up the callstack relative to the selected frame, towards the outermost    \n"
			"call.  A number can be provided which specifies the number of frames to move.  \n"
			"If no argument is given, 'up' moves up by one (1) frame.                       \n\n"
			"SYNTAX:                                                                        \n"
			"    up         - move up by one (1) frame                                      \n"
			"    up <steps> - move up by <steps> frames                                     \n"
		);
	}
	else if (strcmp(command_name, "vars") == 0) {
		printf(
			"Show local variables for the selected stack frame along with all primitive     \n"
			"values.  Non-primitive values such as objects and functions are displayed      \n"
			"simply as '{...}` so you will need to use the `eval` or `examine' commands to  \n"
			"view their content.                                                            \n\n"
			"SYNTAX:                                                                        \n"
			"    vars                                                                       \n"
		);
	}
	else if (strcmp(command_name, "where") == 0) {
		printf(
			"Show the function name, filename and current line number for the innermost     \n"
			"JavaScript stack frame, along with the line of code currently being executed.  \n"
			"This can help you to regain your bearings after an extended debugging session. \n\n"
			"SYNTAX:                                                                        \n"
			"    where                                                                      \n"
		);
	}
	else if (strcmp(command_name, "quit") == 0) {
		printf(
			"Quit SSj.  This will detach the debugger and return you to the shell.  In most \n"
			"cases the neoSphere debug target will also close, unless SSj was started with  \n"
			"the '-a' option to attach to a running instance.                               \n\n"
			"SYNTAX:                                                                        \n"
			"    quit                                                                       \n"
		);
	}
	else if (strcmp(command_name, "help") == 0) {
		printf(
			"Get help with SSj commands.  Since you're seeing this text, it looks like you  \n"
			"already know what to do!                                                       \n\n"
			"SYNTAX:                                                                        \n"
			"    help           - show a list of SSj commands and their functions           \n"
			"    help <command> - get help with a specific command                          \n"
		);
	}
	else
		printf("*munch*\n");
}
