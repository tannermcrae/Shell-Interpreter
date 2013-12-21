
cc = gcc -g
CC = g++ -g

LEX = lex 
YACC = yacc

all: shell

tty-convert-mode.o: tty-convert-mode.c
	$(cc) -c $<

read-line.o: read-line.c
	$(cc) -c $<

lex.yy.o: shell.l
	$(LEX) shell.l
	$(CC) -c lex.yy.c

y.tab.o: shell.y
	$(YACC) -d shell.y
	$(CC) -c y.tab.c

SimpleCommand.o: SimpleCommand.cpp
	$(CC) -c SimpleCommand.cpp

Command.o: Command.cpp
	$(CC) -c Command.cpp

shell: y.tab.o lex.yy.o SimpleCommand.o Command.o tty-convert-mode.o read-line.o
	$(CC) -o ../shell $^ -lfl

clean:
	rm -f lex.yy.c y.tab.c  y.tab.h shell *.o

