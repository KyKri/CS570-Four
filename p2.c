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

void prompt();
void parse();
void sighandler();

int c; /*Number chars per word*/
int numwords; /*Number of words from input line*/
char s[STORAGE]; /*Used to store each word from input*/
char *firstword; /*Points to first word read from input*/
char *lastword; /*Points to last word read from input*/
char *lastword2; /*Points to last word read from input for pipecmd*/
int background; /*set if & is lastword, used to background process*/

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
char *pipecmd; /*points to a command following a pipe*/
int pipeptrerr = 0; /*flag if more than one | detected*/
int pipearg1 = 0;
char *nullfile;

char *newargv[(STORAGE * MAXITEM) + 1]; /*used to send args to children*/
int newargc; /*counts number of args sent to children*/
char *newargv2[(STORAGE * MAXITEM + 1)];
int newargc2; /*counts number of args sent to children*/

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
		/*Shouldnt be here, just testing*/
		//inptr = infile = outfile = outptr = pipeptr = pipecmd = NULL;
		int infiledes = NULL;
		int outfiledes = NULL;
		int nullinput = NULL;

		prompt();
		parse();
		if( /* *firstword == EOF */ c == EOF )
			break;
		if( numwords == 0 )
			continue;
		else{
			if( firstword == NULL ){
				(void) fprintf(stderr,"NULL Command not found.\n");
				continue;
			}/*handle the cd command*/
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
			}/*make sure no ambiguous redirects/pipes detected*/
			if( inptrerr ){
				continue;
			}
			if( outptrerr ){
				continue;
			}
			if( pipeptrerr ){
				continue;
			}
			if( pipeptr != NULL) {
				int pipeopenerr;
				int fildes[2];
				pid_t kidpid1, kidpid2;

				if( (pipeopenerr = (pipe(fildes))) == -1 ){
					perror("Pipe failed!");
					exit(3);
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
/************************kid1********************************/
				fflush(stdin);
				fflush(stdout);
				fflush(stderr);
				if( (kidpid1 = fork()) == -1 ){
					perror("Unable to fork.\n");
					exit (1);
				}else if( kidpid1 == 0 ){
					CHK(dup2(fildes[1], STDOUT_FILENO));
					if( infiledes != NULL ){
						dup2(infiledes, STDIN_FILENO);
						CHK(close(infiledes));
					}
					CHK(close(fildes[0]));
					CHK(close(fildes[1]));
					if( (execvp(newargv[0], newargv)) == -1 ){
						(void) printf("kid1: Command not found.\n");
						exit(2);
					}
				}
				/*else{
					for(;;){
						pid_t pid;
						CHK(pid = wait(NULL));
						if (pid == kidpid1){
							break;
						}
					}
				}*/
/*******************kid 2***************************/
				fflush(stdin);
				fflush(stdout);
				fflush(stderr);
				if( (kidpid2 = fork()) == -1 ){
					perror("Unable to fork.\n");
					exit (1);
				}else if( kidpid2 == 0 ){
					CHK(dup2(fildes[0], STDIN_FILENO));
					if( outfiledes != NULL ){
						dup2(outfiledes, STDOUT_FILENO);
						CHK(close(outfiledes));
					}
					CHK(close(fildes[0]));
					CHK(close(fildes[1]));
					/*testing to give pipe correct args*/
					if( (execvp(newargv2[0]/*pipecmd*/, newargv2/*newargv+pipearg1*/)) == -1 ){
						(void) printf("kid 2: Command not found.\n");
						exit(2);
					}
				}
				/*else{*/
				CHK(close(fildes[0]));
				CHK(close(fildes[1]));
				/*testing lastword2*/
				if ( (strcmp(lastword2, "&")) == 0 /*background*/ ){
					(void) printf("%s [%d]\n", newargv[0], kidpid2);
					/*background /dev/null here?*/
					continue;
				}/*Wait until child finishes*/
					for(;;){
						pid_t pid;
						CHK(pid = wait(NULL));
						if (pid == kidpid2){
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
			fflush(stdin);
			fflush(stdout);
			fflush(stderr);
			int kidpid;
			if( (kidpid = fork()) == -1 ){
				perror("Unable to fork.\n");
				exit (1);
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
					(void) printf("Command not found.\n");
					exit(2);
				}
			}else{
				/*background handler - dont wait for child*/
				if ( (strcmp(lastword, "&")) == 0 /*background*/ ){
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
	exit(0);
}

/*Issues the prompt "p2: " prompting user for input*/
void prompt(){
	(void)printf("p2: ");
}

/*Parse cycles through the input received from stdin by calling
getword.
Parse sets flags when metacharacters are encountered.*/
void parse(){
	numwords = 0;
	newargc = 0;
	newargc2 = 0;
	pipearg1 = 0;
	firstword = NULL;
	lastword = NULL;
	lastword2 = NULL;
	inptr = infile = outfile = outptr = pipeptr = pipecmd = NULL;
	background = 0;
	*lineptr = (int)&line;
	int founddllr = 0;
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
			/*Create a pointer to first char of each word per line*/
			*(word + numwords) = lineptr;
			int i;
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
		/*handle input redirection*/
		if ( (strcmp( word[i], "<")) == 0 ){
			/*If inptr not null, we've already seen one < this line*/
			if ( inptr != NULL ){
				fprintf (stderr,"Error: Ambiguous input\n");
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
					}else if ( word[i+1] == "&" ){
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
				fprintf (stderr, "Error: too many output files\n");
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
					if( word[i+1] == "&" ){
						(void) fprintf (stderr,"Error! Outfile is Null!\n");
						outptrerr = 1;
						break;
					}
					outfile = word[++i];
					lastword = word[i];
				}
			}
		}/*
		else if ( (strcmp( *(word + i), "$")) == 0 ){
			;
		}*//*Makes sure & can be used for backgrounding, but not passed as arg*/
		/*else if ( (strcmp( *(word + i), "&")) == 0 ){
			/*if ( slashfound ){
				printf("slashfound");
			}
			printf("slashfound is: %d\n", slashfound);
			lastword = word[i];
			;
		}*/else if ( (strcmp( *(word + i), "|")) == 0 ){
			if ( pipeptr != NULL ){
				fprintf (stderr,"Error: Too many pipes!\n");
				pipeptrerr = 1;
				break;
			}else{
				pipeptr = word[i];
				lastword = word[i];
				if( i+1 < MAXITEM ){
					if( word[i+1] == NULL ){
						(void) fprintf (stderr,"Error! Pipecmd is Null!\n");
						pipeptrerr = 1;
						break;
					}
					pipecmd = word[++i];
					newargv[++newargc] = NULL;
					newargv2[0] = word[i];
					newargc2++;
					pipearg1 = i;
					lastword = word[i];
				}
			}
		}else{
			if( firstword == NULL ){
				firstword = word[i];
			}/*If we have a pipecmd, the rest of the line goes to pipecmd*/
			else if( pipecmd != NULL){
				/*testing to get pipecmd in newargv2[0]*/
				/*newargv2[newargc2++] = word[i];*/
				newargv2[newargc2++] = word[i];
				newargv2[newargc2] = '\0';
				lastword2 = word[i];
				/*^^may need to become lastword2 ^^*/
			}
			newargv[newargc++] = word[i];
			newargv[newargc] = '\0';
			lastword = word[i];
		}
	}
	if ( c == EOF){
		;
	}
	if (newargv[newargc-1] == NULL){
		;
	}
	else if ( (strcmp(newargv[newargc-1], "&")) == 0 ){
		background = 1;
		newargv[--newargc] = NULL;
	}
	if( newargv2[newargc2-1] == NULL){
		;
	}
	else if (pipecmd != NULL){
		if ( (strcmp(newargv2[newargc2-1], "&")) == 0 ){
			newargv2[--newargc2] = NULL;
		}
	}
}

/*catches the SIGTERM signal to avoid killing p2*/
void sighandler(){
	;
}
