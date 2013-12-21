
/*
 * shell.y: parser for shell
 */

%token	<string_val> WORD

%token 	NOTOKEN GREAT NEWLINE GREATGREAT PIPE AMPERSAND LESS GREATAMPERSAND GREATGREATAMPERSAND ERRTOOUT SEMICOLON SUBSHELL

%union {
	char *string_val;
}

%{
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <regex.h>
#include <assert.h>
#include "Command.h"

#define yylex yylex
#define MAXFILENAME 1024

void yyerror(const char * s);
int yylex();
void expandWildcardsIfNecessary(char *, char *);
void swapUntilEntryInOrder(char**, int);
void makeWildcardArray();
int max_entries;
int n_entries;
char **array;
%}

%%
 
goal: 
	commands ;

commands: 
	command | commands command;

command: 
  simple_command;

simple_command:	
	pipe_list iomodifier_list background_optional NEWLINE {
		//printf("   Yacc: Execute command\n");
		Command::_currentCommand.execute();
	}
	| NEWLINE {
		Command::_currentCommand.prompt();
	}
	| error NEWLINE { 
		yyerrok; 
	};

pipe_list:
	pipe_list PIPE command_and_args {
  	}
  	| command_and_args;

command_and_args:
	command_word arg_list {
		Command::_currentCommand.insertSimpleCommand( Command::_currentSimpleCommand );
	};

arg_list:
	arg_list argument
	| /* can be empty */
	;

argument:
	WORD {
		char *aux = strdup($1);
		// fprintf(stderr, "before aux%s\n", $1 );
		// /* Really crappy way of escaping */ 
		// char *specials = (char *)"\"\\\[\]\^\-\?\.\*\+\|\(\)\$\/\{\}\%\<\>\&";
		// char *copy = strdup($1);
		// char *aux = (char *)malloc(sizeof(char)*strlen(copy));
		// int j = 0;
		// for (int i = 0; i < strlen(copy); i++) {
		// 	if (copy[i] == '\\') {
		// 		for (int k = 0; k < strlen(specials); k++) {
		// 			if (copy[i+1] == specials[k]) {
		// 				aux[j++] = copy[i+1];
		// 				i++;
		// 				break;
		// 			}
		// 		}
		// 	}
		// 	else
		// 		aux[j++] = copy[i];
		// }
		// aux[j] = '\0';
		// fprintf(stderr, "aux %s\n", aux);
		//fprintf(stderr,"%s\n",aux);
		/* End of crappy escaping */

		makeWildcardArray();
		expandWildcardsIfNecessary((char *)"",aux);
		int i;
		for (i = 0; i < n_entries; i++) {
			Command::_currentSimpleCommand->insertArgument(strdup(array[i]));
		}
		free(array);
		free(aux);
	};

command_word:
	WORD {
		Command::_currentSimpleCommand = new SimpleCommand();
		Command::_currentSimpleCommand->insertArgument( $1 );
	};

iomodifier_list:
  iomodifier_list iomodifier_opt
  | /* can be empty */
  ;

iomodifier_opt:
	GREAT WORD {
		//printf("   Yacc: insert output \"%s\"\n", $2);
		if (Command::_currentCommand._outFile)		
			yyerror("Ambiguous output redirect");
		else
			Command::_currentCommand._outFile = $2;
	}

  	| LESS WORD {
   if (Command::_currentCommand._outFile)		
		yyerror("Ambiguous input redirect");
	else
    	Command::_currentCommand._inputFile = $2;
	}

  | GREATAMPERSAND WORD {
    Command::_currentCommand._greatAmpersand = 1;
    Command::_currentCommand._outFile = $2;
    Command::_currentCommand._errFile = $2;
  }

  | GREATGREAT WORD {
    Command::_currentCommand._appendOut = 1;
    Command::_currentCommand._outFile = $2;
  }

  | GREATGREATAMPERSAND WORD {
    Command::_currentCommand._appendAmpersand = 1;
    Command::_currentCommand._outFile = $2;
    Command::_currentCommand._errFile = $2;
  }

    | ERRTOOUT {
  	Command::_currentCommand._errFile = Command::_currentCommand._outFile;
  }

background_optional:
  AMPERSAND {
    Command::_currentCommand._background = 1;
  }
  | /* can be empty */
  ;


%%

void
yyerror(const char * s)
{
	fprintf(stderr,"%s", s);
}

// Specific to array used when sorting wildcard matches
void makeWildcardArray() {
	max_entries = 20;
	n_entries = 0;
	array = (char**)malloc(sizeof(char *)*max_entries);
}

void expandWildcardsIfNecessary(char *prefix, char *suffix) {
	int is_absolute_flag = 0;
	// If there is no wildcards in entry
	if (prefix[0] == '\0' && !strchr(suffix,'*') && !strchr(suffix,'?')) {
		Command::_currentSimpleCommand->insertArgument(strdup(suffix));
		return;
	}
	// Resize the array if it's too small.
	if (n_entries >= max_entries) {
		max_entries *= 2;
		array = (char **)realloc(array, max_entries*sizeof(char *));
		assert(array);
	}

	// Base case: If suffix is empty then everything's been expanded and prefix is word to add to arguments.
	if (suffix[0] == 0) {
		// Suffix is empty. Put prefix in argument.
		array[n_entries] = strdup(prefix);
		swapUntilEntryInOrder(array,n_entries);
		n_entries++;
		return;
	}
	// Obtain the next component in the suffix then advance suffix.
	char *s = strchr(suffix, '/');
	// If the first char in suffix is '/' then search for the next one
	if (s && s-suffix == 0) {
		is_absolute_flag = 1;
		s = strchr(++s, '/');
		suffix++;
	}
	
	char component[MAXFILENAME];
	if (s) {
		strncpy(component,suffix,s-suffix);
		component[s-suffix] = '\0'; 
		suffix = s + 1;
	}
	// Last part of the path. Copy the whole thing
	else {
		strcpy(component, suffix);
		suffix += strlen(suffix);
	}

	/*
	 *  Expand the component
	 */

	 // Check if wildcards exist in component
	char newPrefix[MAXFILENAME];
	if (!strchr(component,'*') && !strchr(component,'?') && !is_absolute_flag) {
		// Handles the empty prefix case and case where component starts wint '/' like /usr
		if (prefix[0] == '\0' || component[0] == '/')
			sprintf(newPrefix,"%s", component);
		else
			sprintf(newPrefix,"%s/%s",prefix, component);
		expandWildcardsIfNecessary(newPrefix,suffix);
		return;
	}
	// Component HAS wildcards

	 
	 /* Begin converting component to regular expression */

	char * reg = (char *)malloc((sizeof(char)*strlen(component)*2)+10);
	int comp_len = strlen(component);
	char * a = component;
	char * r = reg;
	*r = '^';  // Match beginning of the word
	r++;
	while (*a) {
		if (*a == '*') {
			*r = '.'; r++;
			*r = '*'; r++;
		} 
		else if ( *a == '?') { 
			*r = '.'; r++;
		}
		else if (*a == '.') {
			*r = '\\'; r++;
			*r = '.'; r++;
		}
		else if(*a == '/') {
			// Do nothing. Don't want it copied over
		}
		else {
			*r = *a; r++;
		}
		a++;
	}
	*r = '$'; 
	r++;
	*r = '\0';  // Match end of line and add null char

	/* End converting component to regular expression */

	// Create regex and place in regex_t variable.
	regex_t re;	
	int expbuf = regcomp( &re, reg, REG_EXTENDED|REG_NOSUB|REG_NEWLINE);
	if(expbuf != 0) {
		perror("compile");
	}

	// If prefix is empty then list current directory, else list it as prefix
	char *dir;
	if (is_absolute_flag)
		dir = (char *)"/";
	else if (prefix[0]== '\0')
		dir = (char *)".";
	else
		dir=prefix;

	DIR *d = opendir(dir);
	if (d == NULL) {
		// fprintf(stderr, "dir %s\n", dir );
		// perror("opendir");
		return;
	}
	// Check which entries match the regex
	regmatch_t match;
	struct dirent *ent;
	while ((ent = readdir(d)) != NULL) {
		// If entry matches. Add it to the prefix and call expandWildCardsIfNecessary() recursively.
		if (regexec(&re, ent->d_name, 1, &match, 0) == 0) {
			// If the file is invisible (first char is '.'), don't include it 
			// fprintf(stderr, "%s   |   %s\n", ent->d_name, suff );
			if (ent->d_name[0] == '.' && component[0] != '.'){
				// Do nothing
			}
			// if prefix is empty
			else if (prefix[0] == '\0') {
				// For the usr case
				if (dir[0] == '/')
					sprintf(newPrefix, "/%s", ent->d_name);
				else
					sprintf(newPrefix, "%s", ent->d_name);
				expandWildcardsIfNecessary(newPrefix, suffix);
			}
			else {
				sprintf(newPrefix, "%s/%s", prefix, ent->d_name);
				expandWildcardsIfNecessary(newPrefix, suffix);
			}
		}
	}
	closedir(d);
}

void swapUntilEntryInOrder(char** arr, int index) {
	int i;
	char *temp;
	for (i = index; i > 0; i--) {
		if (strcmp(arr[i],arr[i-1]) < 0) {
			temp = arr[i];
			arr[i] = arr[i-1];
			arr[i-1] = temp;
		} 
		else { 
			break;
		}
	}
}


#if 0
main()
{
	yyparse();
}
#endif
