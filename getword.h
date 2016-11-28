/* getword.h - header file for the getword() function used in
   CS570 Fall 2016
   San Diego State University
*/

#include <stdio.h>
#include <string.h>
#include <strings.h>

#define STORAGE 255
      /* This is one more than the max wordsize that getword() can handle */

int getword(char *w);
/* (Note: the preceding line is an ANSI C prototype statement for getword().
    It will work fine with rohan's gcc or cc compiler.)

* The getword() function gets one word from the input stream.
* It returns -1 if end-of-file is encountered;
* otherwise, it returns the number of characters in the word
*
* INPUT: a pointer to the beginning of a character string
* OUTPUT: the number of characters in the word (or the negative of that number)
* SIDE EFFECTS: bytes beginning at address w will be overwritten.
*   Anyone using this routine should have w pointing to an
*   available area at least STORAGE bytes long before calling getword().

Upon return, the string pointed to by w contains the next word in the line
from stdin. A "word" is a string containing a single metacharacter or a string
consisting of non-metacharacters delimited by blanks or metacharacters.

The metacharacters are "<", ">", "|", "$", and "&".
The last word on a line may be terminated by the newline character OR by
end-of-file.

getword() skips leading blanks, so if getword() is called and there are no
more words on the line, then w points to an empty string. All strings,
including the empty string, will be delimited by a zero-byte (eight 0-bits),
as per the normal C convention (this delimiter is not 'counted' when determining
the length of the string that getword will report as a return value).

The backslash character "\" is special, and may change the behavior of
the character that directly follows it on the input line.  When "\" precedes
a metacharacter, that metacharacter is treated like most other characters.
(That is, the symbol will be part of a word rather than a word delimiter.)

Thus, three calls applied to the input
Null&void
will return 4,1,4 and produce the strings "Null", "&", "void", respectively.

However, one call to getword() applied to the input
Null\&void
returns 9 and produces the string "Null&void".
Note that the '\' is NOT part of the resulting string!

Similarly, "\<" is treated as the [non-meta]character "<", "\>" is ">",
"\&" is "&", and "\\" represents the [non-special] character "\".
The combination "\ " should be treated as " ", and therefore allow a
space to be embedded in a word:
Null\ void
returns 9 and produces the string "Null void".  (A backslash preceding
any other character should simply be ignored; in particular, a backslash
before a newline will not affect the meaning of that newline.)

The integer that getword() returns is the length of the resultant
string to which w points. There is one exception to this: If the rest of
the line consists of zero or more blanks followed by end-of-file, then w
still points to an empty string, but the returned integer is (-1).

Example: Suppose the input line were
Hi there&
(Assume there are trailing blanks, followed by EOF.)
Four calls to getword(w) would return 2,5,1,-1 and fill each of the areas
pointed to by w with the strings "Hi", "there", "&", and "", respectively.

Example: Suppose the input line were
Hi there&
(Assume there are two trailing blanks, followed by a newline character.)
Four calls to getword(w) would return 2,5,1,0 and fill each of the
areas pointed to by w with the strings "Hi", "there", "&", and "",
respectively.  (If EOF followed the newline, then a fifth call would
produce "" and return -1.)

Note that we would obtain exactly the same results if the input line had been
    Hi   there  &
(This example has leading blanks and a newline right after the ampersand.)

The specs imply that your code will have to treat spaces differently, depending
on whether they are leading spaces, or whether the space is found when you are
in the 'middle' of a word.  The '$' metacharacter has similar complexities.
If '$' is found in the 'middle' of a word, it is treated like any normal
character (thus, "123$56" will just be a six-letter word).  But when '$'
is the first character of a word, the return value should be negated.
Thus, the four characters "$^*#" will return -4, rather than the usual +4.
Both $abcd and \$abcd produce the string "$abcd", but the first returns -5,
whereas the latter returns +5.

The driver program p1.c is slightly different than p0.c: any negative
return value from getword() caused p0 to quit, but p1 quits ONLY when
it sees the specific value -1.  Note that a lone '$' is a length-one word,
but the [leading] '$' negates the one, so "$" returns -1, too -- and
so "$" will cause p1 to exit when it sees this return value.

If the word scanned is longer than STORAGE-1, then getword() constructs the
string consisting of the first STORAGE-1 bytes only. (As usual, a zero-byte
is appended. The next getword() call will begin with the rest of that word.)

Useful manpages to consider are those for ungetc() and getchar().

*/

