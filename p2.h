/*Header file for p2 CS570 Fall 2016 SDSU
This file includes necessary files and makes
definitions for use in the program*/
#include <stdio.h>
#include "getword.h"
#define MAXITEM 100 /*max number of words allowed per line*/
int slashfound;

/*p2 should prompt for input from stdin
p2 accepts 0 or more words per line.
If it receives a 0 word line, it reissues the prompt.
p2 terminates if EOF is the first word read.
For other input, p2 treats it as an executable file
and tries to execute it.
p2 should warn if a file does not exist, or is not executable
and it should not crash.
p2 handles standard shell redirection using < > and |
p2 will warn if a file listed for redirection already exists.
p2 only handles one pipe, two | read on one line
results in an error message.
p2 will give an error if asked to send redirection to two files.
p2 sends an error message for ambiguous inputs.
p2, upon reading & at the end of a line, will start a new
process to execute the file, print out its pid and contimue
without waiting for the childs completion.
p2 handles the cd command like other shells*/
