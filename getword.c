/*Programmer: Kyle Krick
Class: CS570 Operating Systems
Professor: John Carroll
Semester: Fall 2016
Due Date: 09/21/16*/
#include "getword.h"
#include "p2.h"

#define SPACE ' '
#define NEWLINE '\n'
#define LSSR '<'
#define GRTR '>'
#define PIPE '|'
#define DLLR '$'
#define AMP '&'
#define BACK '\\'

/* The getword() function gets one word from the input stream.
* It returns -1 if end-of-file is encountered;
* otherwise, it returns the number of characters in the word
*
* INPUT: a pointer to the beginning of a character string
* OUTPUT: the number of characters in the word (or the negative of that number)
* SIDE EFFECTS: bytes beginning at address w will be overwritten.
*   Anyone using this routine should have w pointing to an
*   available area at least STORAGE bytes long before calling getword().
*/
int getword(char *w){
	/*numletters is the number of chars in current word*/
	/*dllrfrst is set when DLLR is read, every return numletter
	checks to see if dllrfrst is set, if it is, negate numletters*/
	int curchar = 0, numletters = 0, dllrfrst = 0;
	slashfound = -1;

	while ( (curchar = getchar()) != EOF ){
		/*Not processing word, newline entered - null terminate
		string - return 0*/
		if( (numletters == 0) && ((char)curchar == NEWLINE) ){
			*w = '\0';
			return 0;
		}
		/*Processing word, newline delimeter encountered -
		return size*/
		else if( (char)curchar == NEWLINE ){
			ungetc(NEWLINE , stdin);
			if( dllrfrst )
				numletters = numletters * -1;
			return numletters;
		}
		/* '\' handling*/
		else if ( (char)curchar == BACK ){
			curchar = getchar();
			/*We simply ungetc because we already have code to handle EOF*/
			if( (char)curchar == EOF){
				ungetc(EOF , stdin);
			}
			/*We simply ungetc because we already have code to handle NEWLINE*/
			else if ( (char)curchar == NEWLINE ){
				ungetc(NEWLINE , stdin);
			}
			/*Checks to make sure the char after the BACK isn't going over
			size constraints*/
			else if (numletters == STORAGE-1){
				ungetc(curchar, stdin);
				if( dllrfrst )
					numletters = numletters * -1;
				return numletters;
			}
			else{
				if( (numletters == 0) && ((char)curchar == DLLR) ){
					dllrfrst=1;
				}
				*w++ = curchar;
				*w = '\0';
				numletters++;
				slashfound = numletters-1;
			}
		}
		/*metachar is a word by itself*/
		else if ( (numletters == 0) && (((char)curchar == LSSR)
|| ((char)curchar == GRTR) || ((char)curchar == PIPE) || ((char)curchar == AMP)) ){
			*w++ = curchar;
			*w = '\0';
			return 1;
		}
		/*metachar delimits a word*/
		else if ( ((char)curchar == LSSR)
|| ((char)curchar == GRTR) || ((char)curchar == PIPE) || ((char)curchar == AMP) ){
			ungetc(curchar , stdin);
			if( dllrfrst )
				numletters = numletters * -1;
			return numletters;
		}
		/*size > 0 means not leading blank, blank delimeter
		encountered - return size*/
		else if( ((char)curchar == SPACE) && (numletters > 0) ){
			if( dllrfrst )
				numletters = numletters * -1;
			return numletters;
		}
		else{
			/*leading blank, do not add to string*/
			if( ((char)curchar == SPACE) && (numletters == 0) ){
				;
			}
			/*Make sure we dont go over the size constraint if we do,
			then put the char back and return the current word*/
			else if (numletters == STORAGE-1){
				ungetc(curchar, stdin);
				if( dllrfrst )
					numletters = numletters * -1;
				return numletters;
			}
			/*curchar not a delimeter - add curchar to string*/
			else{
				if( (numletters == 0) && ((char)curchar == DLLR) ){
					dllrfrst=1;
				}
				*w++ = curchar;
				*w = '\0';
				numletters++;
			}
		}
	}/*while*/

	/*EOF encountered, if in middle of word return the word, put EOF
	back so that we immediately quit upon the next call*/
	if((char)curchar == EOF){
		if( numletters > 0 ){
			if( dllrfrst )
				numletters = numletters * -1;
			return numletters;
		}else{
			*w++ = '\0';
			return -1;
		}
	}
}
