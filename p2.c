/*P2
Programmer: Kyle Krick
Due: 10/12/16
CS570
Fall 2016
John Carroll*/

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include "p2.h"
#include "/home/ma/cs570/CHK.h"

#define SPACE ' '
#define NEWLINE '\n'
#define LSSR '<'
#define GRTR '>'
#define PIPE '|'
#define DLLR '$'
#define AMP '&'
#define BACK '\\'
#define MAXARGS 20
#define MAXPIPES 10 /*maximum number of pipes that p2 will allow*/

void prompt();
void parse();
void sighandler();
void cleararray (char *[]);
void closefildes (int [], int);

int c; /*Number chars per word*/
int numwords; /*Number of words from input line*/
char s[STORAGE]; /*Used to store each word from input*/
char *firstword; /*Points to first word read from input*/
char *lastword; /*Points to last word read from input*/

/*Storage * Maxitem because each word is a max size of storage and
each line has a max word count of maxtitem therefore, the biggest
possible line is (storage * maxitem) (+ 1 for null terminator)*/
char line[(STORAGE * MAXITEM) + 1]; /*Stores line read from input*/
char *lineptr = &line[0]; /*Used to cycle through each word*/
char *word[MAXITEM]; /*Used to mark the start of each word per line*/

char *inptr; /*points to an input redirect received in input line*/
int inptrerr = 0; /*flag if ambiguous input detected*/
char *infile; /*points to filename for input*/
char *outptr; /*points to an output redirect received in input line*/
int outptrerr = 0; /*flag if ambiguous output detected*/
char *outfile; /*Points to filename for output*/
char *pipeptr; /*points to a pipe received in input line*/
int pipeptrerr = 0; /*flag if more than one | detected*/
char *nullfile;
int dllrworderr = 0; /*Used for errors with invalid environment variables*/

char *newargv[(STORAGE * MAXITEM) + 1]; /*used to send args to children*/
char *newargv2[(STORAGE * MAXITEM) + 1]; /*used to send args to children*/
int newargc;
int newargi[MAXPIPES]; /*stores the index of each pipe's first arg, which
are all stored in newargv*/
int numpipes; /*Used to count the number of pipes read per line*/

/*Main prompts for input, handles EOF, handles creating new
processes, handles redirection and kills children.
Exits with code 0 if no errors.*/
int main(){
	/*dont want to kill ourself! send SIGTERM to a handler*/
	signal(SIGTERM, sighandler);

	for(;;){
		inptrerr = 0;
		outptrerr = 0;
		pipeptrerr = 0;
		numpipes = 0;

		int infiledes = NULL;
		int outfiledes = NULL;
		int nullinput = NULL;

		prompt();
		parse();
		cleararray(word);
		if( c == EOF )
			break;
		if( numwords == 0 )
			continue;
			/*If a word starting with $ wasn't a valid env var,
			we don't want to fork a child*/
		else if (dllrworderr)
			continue;
		else{
			if( firstword == NULL ){
				(void) fprintf(stderr,"NULL Command not found.\n");
				continue;
			}
/*************************** handle cd *************************************/
			else if( (strcmp(firstword, "cd")) == 0 ){
				/*Make sure cd has only 1 arg*/
				if( newargc > 2 ){
					(void) fprintf(stderr,"cd received too many arguments\n");
					continue;
				}/*If cd has no arguments, set its path to $HOME*/
				else if (newargv[1] == NULL){
					/*make sure HOME is defined*/
					if ( (getenv("HOME")) == NULL ){
						(void) fprintf(stderr,"HOME variable not defined.\n");
						continue;
					}else{
						chdir(getenv("HOME"));
						continue;
					}
				}else if( (chdir(newargv[1])) == -1 ){
					(void) fprintf(stderr,"No such file or directory.\n");
					continue;
				}
				continue;
			}
/*************************** handle environ **********************************/
			/*environ allows for reading environment variables when given
			one argument and allows for setting environment variables when
			given two arguments, fails if given no args or 3+ args*/
			else if ( (strcmp(firstword, "environ")) == 0 ){
				if ( newargc < 2 ){
					(void) fprintf(stderr,"environ needs at least 1 arg\n");
				}else if ( newargc > 3 ){
					(void) fprintf(stderr,"environ handles only 1 or 2 args\n");
				}/*We have one arg, try to read its value and print,
				print nothng if not found*/
				else if ( newargc == 2 ){
					if ( getenv(newargv[newargc-1]) == NULL ){
						(void) printf("\n");
					}else{
						(void) printf("%s\n", getenv(newargv[newargc-1]));
					}
				}/*We have 2 args try to assign new value*/
				else{
					if ( (setenv(newargv[newargc-2], newargv[newargc-1], 1)) == -1 ){
						(void) fprintf(stderr,"failed to assign env var\n");
					}
				}
				continue;
			}
/****************make sure no ambiguous redirects/pipes detected**************/
			if( inptrerr ){
				continue;
			}
			if( outptrerr ){
				continue;
			}
			if( pipeptrerr ){
				continue;
			}
/**************************** handle pipe ***********************************/
			if( numpipes > 0 ) {
				int pipeopenerr;
				int fildes[numpipes*2];
				pid_t kidpids[numpipes+1];
				/*TO-DO make pipe loop that opens
				all pipes individually ie fildes[0] and [1] then [2] and [3]*/
				int j;
				for ( j = 0; j < numpipes*2 ; j+=2 ){
					if( (pipeopenerr = (pipe(fildes+j))) == -1 ){
						perror("Pipe failed!");
						_exit(3);
					}
				}
				/*Check for an infile, try to open*/
				if( infile != NULL ){
					infiledes = open(infile, O_RDONLY);
					if( infiledes == -1 ){
						(void) fprintf(stderr,"Error: Can't read infile!\n");
						continue;
					}
				}
				/*Check for an outfile, try to create*/
				if( outfile != NULL ){
					outfiledes = open( outfile, O_WRONLY | O_EXCL | O_CREAT, 0600 );
					if( outfiledes == -1 ){
						(void) fprintf(stderr,"Error: Can't open outfile!\n");
						continue;
					}
				}
/***************************** first pipe *************************************/
				fflush(stdout);
				fflush(stderr);
				if( (kidpids[0] = fork()) == -1 ){
					perror("Unable to fork.\n");
					_exit (1);
				}else if( kidpids[0] == 0 ){
					CHK(dup2(fildes[1], STDOUT_FILENO));
					if( infiledes != NULL ){
						dup2(infiledes, STDIN_FILENO);
						CHK(close(infiledes));
					}/*TESTING - closefildes function*/
					/*CHK(close(fildes[0]));
					CHK(close(fildes[1]));*/
					closefildes(fildes, numpipes*2);
					if( (execvp(newargv[0], newargv)) == -1 ){
						(void) fprintf(stderr,"kid1: Command not found.\n");
						_exit(2);
					}
				}
/*************************** handle multiple pipes ****************************/
				int i;
				for ( i = 1; i < numpipes; i++){
					fflush(stdout);
					fflush(stderr);
					if( (kidpids[i] = fork()) == -1 ){
						perror("Unable to fork.\n");
						_exit (1);
					}else if (kidpids[i] == 0){
						CHK(dup2(fildes[2*i-2], STDIN_FILENO));
						CHK(dup2(fildes[2*i+1], STDOUT_FILENO));
						closefildes(fildes, numpipes*2);
						if( (execvp(newargv[newargi[i]], (newargv+newargi[i]))) == -1 ){
							(void) fprintf(stderr,"kid%d: Command not found.\n",i);
							_exit(2);
						}
					}
				}
/*********************************** last pipe ********************************/
				fflush(stdout);
				fflush(stderr);
				if( (kidpids[numpipes] = fork()) == -1 ){
					perror("Unable to fork.\n");
					_exit (1);
				}else if( kidpids[numpipes] == 0 ){
					/*TESTING*/
					CHK(dup2(fildes[2*numpipes-2], STDIN_FILENO));
					if( outfiledes != NULL ){
						dup2(outfiledes, STDOUT_FILENO);
						CHK(close(outfiledes));
					}/*TESTING - closefildes function*/
					/*CHK(close(fildes[0]));
					CHK(close(fildes[1]));*/
					closefildes(fildes, numpipes*2);
					if( (execvp(newargv[newargi[numpipes-1]], (newargv+newargi[numpipes-1]) )) == -1 ){
						(void) fprintf(stderr,"kid 2: Command not found.\n");
						_exit(2);
					}
				}
				/*else{*/
				/*TESTING - closefildes function*/
				/*CHK(close(fildes[0]));
				CHK(close(fildes[1]));*/
				closefildes(fildes, numpipes*2);
				if ( (strcmp(lastword, "&")) == 0 ){
					(void) printf("%s [%d]\n", newargv[newargi[numpipes-1]], kidpids[numpipes]);
					/*background /dev/null here?*/
					continue;
				}/*Wait until child finishes*/
					for(;;){
						pid_t pid;
						CHK(pid = wait(NULL));
						if (pid == kidpids[numpipes]){
							break;
						}
					}
				/*}*/
				continue;
			}
/***********************************pipe end**************************/

			/*Check for an infile, try to open*/
			if( infile != NULL ){
				infiledes = open(infile, O_RDONLY);
				if( infiledes == -1 ){
					(void) fprintf(stderr,"Error: Can't read infile!\n");
					continue;
				}
			}
			/*Check for an outfile, try to create*/
			if( outfile != NULL ){
				outfiledes = open( outfile, O_WRONLY | O_EXCL | O_CREAT, 0600 );
				if( outfiledes == -1 ){
					(void) fprintf(stderr,"Error: Can't open outfile!\n");
					continue;
				}
			}
			if( (strcmp(lastword, "&")) == 0 ){
				nullfile = "/dev/null";
				nullinput = open(nullfile, O_RDONLY);
				if( nullinput == -1 ){
					(void) fprintf(stderr,"Error: Can't read nullfile!\n");
					continue;
				}
			}
			fflush(stdout);
			fflush(stderr);
			int kidpid;
			if( (kidpid = fork()) == -1 ){
				perror("Unable to fork.\n");
				_exit (1);
			}else if( kidpid == 0 ){
				if( infiledes != NULL ){
					dup2(infiledes, STDIN_FILENO);
					CHK(close(infiledes));
				}
				if( outfiledes != NULL ){
					dup2(outfiledes, STDOUT_FILENO);
					CHK(close(outfiledes));
				}
				if( ((strcmp(lastword, "&")) == 0) && (infiledes == NULL) ){
					dup2(nullinput,STDIN_FILENO);
					CHK(close(nullinput));
				}
				if( (execvp(newargv[0], newargv)) == -1 ){
					(void) fprintf(stderr,"Command not found.\n");
					_exit(2);
				}
			}else{
				/*background handler - dont wait for child*/
				if ( (strcmp(lastword, "&")) == 0 ){
					(void) printf("%s [%d]\n", newargv[0], kidpid);
					/*background /dev/null here?*/
					continue;
				}/*Wait until child finishes*/
				else{
					for(;;){
						pid_t pid;
						CHK(pid = wait(NULL));
						if (pid == kidpid){
							break;
						}
					}
				}
			}
		}
	}
	killpg(getpid(), SIGTERM);
	(void) printf("p2 terminated.\n");
	_exit(0);
}

/******************************** prompt ***********************************/
/*Issues the prompt "p2: " prompting user for input*/
void prompt(){
	(void)printf("p2: ");
}

/******************************** parse *************************************/
/*Parse cycles through the input received from stdin by calling
getword.
Parse sets flags when metacharacters are encountered.*/
void parse(){
	numwords = 0;
	newargc = 0;
	numpipes = 0;
	firstword = NULL;
	lastword = NULL;
	inptr = infile = outfile = outptr = pipeptr = NULL;
	*lineptr = (int)&line;
	int founddllr = 0;
	dllrworderr = 0;
	/*this loop adds words to the line buffer until c is EOF
	or 0 (meaning getword hit s newline)*/
	for(;;){
		if(founddllr)
			break;
		c = getword(s);
		/*Handle $ as EOF*/
		if( c == -1 ){
			if( (s[0]=='$') && (numwords > 0) ){
				ungetc('$', stdin);
				/*Make sure it can get added to the line*/
				c = 1;
				founddllr = 1;
			}
			else{
				break;
			}
		}
		if( c == 0 )
			break;
		else{
			/*Stop p2 from taking more than MAXITEM words per line*/
			if( numwords+1 == MAXITEM ){
				(void) fprintf(stderr,"Too many args.\n");
				break;
			}
			/*Checking for word starting with $ (getword returns negative
			word count for words starting with $)*/
			if ( c < -1){
				/*check to make sure environment variable exists, s is used
				because we don't want to add a bad env var to newargv. s
				marks the $, so s+1 gives the actual variable starting char*/
				if ( getenv(s+1) == NULL ){
					(void) fprintf(stderr, "Environment Variable doesn't exist.\n");
					dllrworderr = 1;
					/*break; *//*Was in originally, taken out in testing,
					doesn't appear to break anything*/
				}
				/*If the env var exists, copy it in place of the $word we found
				then count to find it's length and store that in c. c is used
				below to add items to the word array*/
				else{
					strncpy(s, getenv(s+1), sizeof(s)-1);
					int i;
					int j = 0;
					for (i = 0; i < sizeof(s)/sizeof(s[0]); i++){
						if( s[i] != '\0' ){
							j++;
						}
					}
					c = j;
				}
			}
			/*Create a pointer to first char of each word per line*/
			*(word + numwords) = lineptr;
			int i;
			/*Add all of the characters of the word in the s array
			to the line array*/
			for( i=0; i<c; i++){
				*lineptr++ = s[i];
				*lineptr = '\0';
			}
			*lineptr++ = '\0';
			numwords++;
		}
	}

	/*this loop cycles through each word in the line and sets
	appropriate flags
	it also does error checking*/
	int i;
	for ( i=0; i < numwords; i++){
		/*If a word starting with $ wasn't a valid env var,
		we don't want to continue parsing*/
		if (dllrworderr)
			break;
		/*handle input redirection*/
		if ( (strcmp( word[i], "<")) == 0 ){
			/*If inptr not null, we've already seen one < this line*/
			if ( inptr != NULL ){
				(void) fprintf (stderr,"Error: Ambiguous input\n");
				inptrerr = 1;
				break;
			}else{
				inptr = word[i];
				lastword = word[i];
				if( i+1 < MAXITEM ){
					if( word[i+1] == NULL ){
						(void) fprintf (stderr,"Error! Infile is Null!\n");
						inptrerr = 1;
						break;
					}else if ( strcmp(word[i+1], "&") == 0 ){
						(void) fprintf (stderr,"Error! Infile is Null!\n");
						inptrerr = 1;
						break;
					}
					infile = word[++i];
					lastword = word[i];
				}
			}
		}/*handle output redirection*/
		else if ( (strcmp( *(word + i), ">")) == 0 ){
			/*If outptr not null, we've already seen one > this line*/
			if ( outptr != NULL ){
				(void) fprintf (stderr, "Error: too many output files\n");
				outptrerr = 1;
				break;
			}else{
				outptr = word[i];
				lastword = word[i];
				if( i+1 < MAXITEM ){
					if( word[i+1] == NULL ){
						(void) fprintf (stderr,"Error! Outfile is Null!\n");
						outptrerr = 1;
						break;
					}
					if( strcmp(word[i+1], "&") == 0 ){
						(void) fprintf (stderr,"Error! Outfile is Null!\n");
						outptrerr = 1;
						break;
					}
					outfile = word[++i];
					lastword = word[i];
				}
			}
		}
/********************************* handle pipes ***************************/
		/*Here we detect pipes and set up newargi as needed, each new newargv
		remains in newargv, but we set an int array to track the pipes first
		argument*/
		else if ( (strcmp( *(word + i), "|")) == 0 ){
			/*Check to make sure we don't have too many pipes*/
			if ( numpipes == MAXPIPES ){
				(void) fprintf (stderr, "Error: Too many pipes!\n");
				pipeptrerr = 1;
			}/*Check to make sure the pipe has an arg*/
			else if ( word[i+1] == NULL ) {
				(void) fprintf (stderr, "Error: No arg after pipe!\n");
				pipeptrerr = 1;
			}
			else if ( strcmp(word[i+1], "&") == 0 ) {
				(void) fprintf (stderr, "Error: No arg after pipe!\n");
				pipeptrerr = 1;
			}/*We've got a pipe with at least one arg! track where its arg is in
			newargv and put a null in newargv in place of the pipe
			(this separates the different newargv's)*/
			else{
				newargi[numpipes] = newargc+1;
				newargv[newargc] = NULL;
				newargv[++newargc] = '\0';
				numpipes++;
			}
		}else{
			if( firstword == NULL ){
				firstword = word[i];
			}
			newargv[newargc++] = word[i];
			newargv[newargc] = '\0';
			lastword = word[i];
		}
	}
	if ( c == EOF){
		;
	}
	/*Make sure that & does not get left on the end of newargv,
	if its on the end it is used for backgrounding and should
	not be added*/
	if (newargv[newargc-1] == NULL){
		;
	}
	else if ( (strcmp(newargv[newargc-1], "&")) == 0 ){
		newargv[--newargc] = NULL;
	}
}

/*catches the SIGTERM signal to avoid killing p2*/
void sighandler(){
	;
}

/*!!!Note!!! specialized code! Do not reuse as is!
Sets all of the pointers from ptrrarray to null
This is ONLY used on the word array for this program*/
void cleararray (char *ptrarray[]){
    int i;
    for ( i = 0; i < numwords; i++ ){
        ptrarray[i] = '\0';
    }
}

void closefildes (int fildesarray[], int numelms){
    int i = 0;
    for (i = 0; i < numelms; i++){
        CHK(close(fildesarray[i]));
    }
}
