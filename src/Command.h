
#ifndef Command_h
#define Command_h

#include "SimpleCommand.h"

struct Command {
	int _numberOfAvailableSimpleCommands;
	int _numberOfSimpleCommands;
	SimpleCommand ** _simpleCommands;
	char * _outFile;
	char * _inputFile;
	char * _errFile;
	int _background;
	int _appendOut;
	int _greatAmpersand;
	int _appendAmpersand;

	char *shellPath;

	void prompt();
	void execute();
	void clear();
	
	Command();
	void insertSimpleCommand(SimpleCommand * simpleCommand);

	static Command _currentCommand;
	static SimpleCommand *_currentSimpleCommand;
};

#endif
