#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <ctype.h>

#include "SimpleCommand.h"

SimpleCommand::SimpleCommand()
{
	// Creat available space for 5 arguments
	_numberOfAvailableArguments = 5;
	_numberOfArguments = 0;
	_arguments = (char **) malloc(_numberOfAvailableArguments * sizeof(char *));
}

void SimpleCommand::insertArgument(char * argument)
{
	if (_numberOfAvailableArguments == _numberOfArguments + 1) {
		// Double the available space
		_numberOfAvailableArguments *= 2;
		_arguments = (char **) realloc( _arguments,
				  _numberOfAvailableArguments * sizeof(char *));
	}

	// Tilde Expansion
	if (argument[0] == '~') {
		if (strlen(argument) == 1)
			argument = strdup(getenv("HOME"));
		else {
			char *home;
			if (isalpha(argument[1]))
				home = (char *)"/homes/";
			else
				home = getenv("HOME");
			char *newArg = (char *)malloc(sizeof(char)*(strlen(argument)+ strlen(home)));
			strcpy(newArg,home);
			strcat(newArg,argument+1);
			argument = strdup(newArg);
			free(newArg);
		}
	}

	// Expand variables if necessary
	char reg[] = "^.*${[^}][^}]*}.*$";
	regex_t re;	
	regmatch_t match;
	int expbuf = regcomp( &re, reg, REG_NOSUB);
	while (regexec(&re, argument, 1, &match, 0) == 0) {
		char *beginning = strchr(argument, '$');
		char *end = strchr(argument,'}');
		// -2 for the $ and { symbol
		int varSize = end - beginning - 2;
		char *var = (char *)malloc(sizeof(char)*(varSize+2));
		strncpy(var,beginning+2,varSize);
		var[(end-beginning)-2] = '\0';
		char * enVar = getenv(var);
		char *newArg = (char *)malloc(sizeof(char)*(strlen(argument)+strlen(enVar)));
		strncpy(newArg,argument,beginning-argument);
		newArg[beginning-argument] = '\0';
		strcat(newArg,enVar);
		strcat(newArg,end+1);
		//fprintf(stderr, "%s\n", newArg );
		argument = strdup(newArg);
		free(newArg);
		free(var);
	}
	
	_arguments[ _numberOfArguments ] = argument;

	// Add NULL argument at the end
	_arguments[ _numberOfArguments + 1] = NULL;
	
	_numberOfArguments++;
}