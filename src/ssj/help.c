#include "ssj.h"
#include "help.h"

void
print_help(const char* command_name)
{
	if (command_name == NULL || strcmp(command_name, "") == 0) {
		printf(
			"Abbreviated names are listed first, followed by the full, verbose name of each \n"
			"command. Unlike GDB, truncated names are not allowed.                          \n\n"

			" bt, backtrace    Show a list of all function calls currently on the stack     \n"
			" d,  down         Move down the call stack (inwards) from the selected frame   \n"
			" bp, breakpoint   Set a breakpoint at file:line (e.g. scripts/eaty-pig.js:812) \n"
			" cb, clearbreak   Clear a breakpoint set at file:line (see 'breakpoint')       \n"
			" c,  continue     Run either until a breakpoint is hit or an error is thrown   \n"
			" e,  eval         Evaluate a JavaScript expression                             \n"
			" x,  examine      Like 'eval' but shows low-level runtime metadata for objects \n"
			" f,  frame        Select the stack frame used for, e.g. 'eval' and 'var'       \n"
			" l,  list         Show source text around the line of code being debugged      \n"
			" s,  stepover     Run the next line of code                                    \n"
			" si, stepin       Run the next line of code, stepping into functions           \n"
			" so, stepout      Run until the current function call returns                  \n"
			" v,  vars         List local variables and their values in the active frame    \n"
			" u,  up           Move up the call stack (outwards) from the selected frame    \n"
			" w,  where        Show the filename and line number of the next line of code   \n"
			" h,  help         Get help with SSJ commands                                   \n"
			" q,  quit         Detach and terminate your SSJ debugging session              \n\n"

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
	else if (strcmp(command_name, "eval") == 0) {
		printf(
			"Evaluate a JavaScript expression. This works exactly like JavaScript eval() and\n"
			"allows you to examine objects, assign to variables, or anything else you could \n"
			"do with a line of code.                                                        \n\n"
			"SYNTAX:                                                                        \n"
			"    eval <expression>                                                          \n"
		);
	}
}
