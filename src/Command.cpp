
/*
 * Implements the class and struct defined in command.h
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <regex.h>
#include <ctype.h>
#include "Command.h"

void sigHandler(int);
struct sigaction sigAction;

Command::Command()
{
	// Create available space for one simple command
	_numberOfAvailableSimpleCommands = 1;
	_simpleCommands = (SimpleCommand **)
		malloc( _numberOfSimpleCommands * sizeof( SimpleCommand * ) );

	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;
}

void Command::insertSimpleCommand( SimpleCommand * simpleCommand )
{
	if ( _numberOfAvailableSimpleCommands == _numberOfSimpleCommands ) {
		_numberOfAvailableSimpleCommands *= 2;
		_simpleCommands = (SimpleCommand **) realloc( _simpleCommands,
			 _numberOfAvailableSimpleCommands * sizeof( SimpleCommand * ) );
	}
	
	_simpleCommands[ _numberOfSimpleCommands ] = simpleCommand;
	_numberOfSimpleCommands++;
}

void Command::clear()
{
	for ( int i = 0; i < _numberOfSimpleCommands; i++ ) {
		for ( int j = 0; j < _simpleCommands[ i ]->_numberOfArguments; j ++ ) {
			free ( _simpleCommands[ i ]->_arguments[ j ] );
		}
		
		free ( _simpleCommands[ i ]->_arguments );
		free ( _simpleCommands[ i ] );
	}
  
  /* checks if >& was used to get rid of double free error */
  if (_outFile == _errFile) {
    free( _outFile);
  }
  else {
	  if ( _outFile ) {
		free( _outFile );
	  }
	  if ( _errFile ) {
		  free( _errFile );
	  }
  }
  if ( _inputFile ) {
    free( _inputFile );
  } 

	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;
}

void
Command::execute()
{
	// Don't do anything if there are no simple commands
	if ( _numberOfSimpleCommands == 0 ) {
		clear();
		prompt();
		return;
	}

	// Print contents of Command data structure
	// print();

	// Add execution here
	// For every simple command fork a new process
	// Setup i/o redirection
	// and call exec
	int tmpin = dup(0);
	int tmpout = dup(1);
	int tmperr = dup(2);
	//set the initial input
	int fdin;
	if (_inputFile) {
		fdin = open(_inputFile, O_RDWR, S_IRUSR | S_IWUSR | S_IXUSR);
	}
	else {
		// Use default input
		fdin = dup(tmpin);
	} 
	int ret;
	int i;
	int fdout;
	int fderr;

	/* If the error file exists, then create an appropriate file descriptor
	 * based off the privedges it needs. Note that errFile is the same as
	 * outFile so fdout can be set to ferr if errFile exists. 
	*/
    if(_errFile) {
    	//no append. set error to be able to create and delete stuff in file
    	if (_greatAmpersand)
        	fderr = open(_errFile, O_TRUNC | O_CREAT | O_RDWR, 0666);
        //only other option is to append
        else
        	fderr = open(_errFile, O_CREAT | O_RDWR | O_APPEND, 0666);
    }
    //else it's standard output
    else {
        fderr = dup(tmperr);
    }
    // redirect fderr into the fd
    dup2(fderr, 2);
    close(fderr);
 
	for (i = 0; i < _numberOfSimpleCommands; i++) {
		// redirect input
		dup2 (fdin, 0);
		close(fdin);

		if (!strcmp(_simpleCommands[i]->_arguments[0],"exit")) {
   			_exit(1);       
   		}
		//setup output and error 
		if ( i == _numberOfSimpleCommands-1) {
			//if redirect out and err to same file
			if (_greatAmpersand && _outFile && _errFile) 
				fdout = fderr; //already defined with fderr
			// else if append output to existing file. 
			else if (_appendAmpersand && _outFile && _errFile)
				fdout = fderr; //already defined with fderr
			// append out file when no error file defined
			else if (_outFile && _appendOut)
				fdout = open(_outFile, O_CREAT | O_APPEND | O_RDWR);
			//output if not being appended
			else if (_outFile) 
				fdout = open(_outFile, O_CREAT | O_TRUNC | O_RDWR, 0666);
			else {
				// Use default output
				fdout = dup(tmpout);
			}
		}
		//Not last simple command
		else {
			//create pipe
			int fdpipe[2];
			pipe(fdpipe);
			fdout = fdpipe[1];
			fdin = fdpipe[0];
		}
		dup2(fdout,1);
		close(fdout);

		// This code handle cd
		if (!strcmp(_simpleCommands[i]->_arguments[0],"cd")) {
			char *arg = _simpleCommands[i]->_arguments[1];
			char *cmd = _simpleCommands[i]->_arguments[0];
			if (!arg) 
				chdir(getenv("HOME"));
			else if (chdir(arg) == -1)
				fprintf(stderr, "%s\n", "No such file or directory" );
			continue;
		}

		// This code handles setenv
        if(!strcmp(_simpleCommands[i]->_arguments[0],"setenv")) {
			char *arg1 = _simpleCommands[i]->_arguments[1];
			char *arg2 = _simpleCommands[i]->_arguments[2];
		    if(arg1 && arg2)
		    	setenv(arg1,arg2,1);
		    else
		    	setenv(arg1,"",1);
		    continue;
        }

        if(!strcmp(_simpleCommands[i]->_arguments[0],"unsetenv")) {
        	char *arg = _simpleCommands[i]->_arguments[1];
        	if (arg) {
        		unsetenv(arg);
        	}
        	continue;
        }
		//create child process
		ret = fork();
		if (ret == 0) {
			if (!strcmp(_simpleCommands[i]->_arguments[0],"printenv")) {
            	char** p = environ;
            	while(*p != NULL){
                	printf("%s\n", *p);
                    p++;
                }
        		exit(0);
            }
            
			//child
			execvp(_simpleCommands[i]->_arguments[0], _simpleCommands[i]->_arguments);
			perror("execvp");
			_exit(1);
		}

		else if (ret < 0) { 
			perror("fork");
			return;
		}
	}
	//restore in/out defaults
	dup2(tmperr,2);
	dup2(tmpout,1);
	dup2(tmpin,0);
	close(tmpin);
	close(tmpout);
	close(tmperr);
	if (!_background) {
		// wait for last process
		waitpid(ret, NULL, 0); 
		_background = 0;
	}

	//Take care of zombie
	//struct sigaction signalAction;
	int z_error = sigaction(SIGCHLD, &sigAction, NULL);
	if ( z_error ) {
		perror( "sigaction" );
		exit(-1);
	}

	// Clear to prepare for next command
	clear();
	
	// Print new prompt
  	prompt();
}

// Shell implementation

void
Command::prompt()
{ 
	if (isatty(0)) {
		printf("myshell> ");
	}
	fflush(stdout);  
}

Command Command::_currentCommand;
SimpleCommand * Command::_currentSimpleCommand;

int yyparse(void);

void sigHandler(int sig) {
	while(waitpid(-1, NULL, WNOHANG) > 0);
	if (sig == SIGINT) {
		printf("\n");
		Command::_currentCommand.prompt();
	}
}

int main(int argc, char const *argv[]) {
	char buff[200];
	realpath(argv[0], buff);
	Command::_currentCommand.shellPath = buff;
	sigAction.sa_handler = sigHandler;
	sigemptyset(&sigAction.sa_mask);
	sigAction.sa_flags = SA_RESTART;
	int error = sigaction(SIGINT, &sigAction, NULL );
	if ( error ) {
		perror( "sigaction" );
		exit(-1);
	}

	Command::_currentCommand.prompt();
	yyparse();
}

