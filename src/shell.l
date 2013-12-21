
/*
 * shell.l: lexical analyzer for shell
 */

%{

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "y.tab.h"
#include "Command.h"

/* Added code for the line editor */
extern "C" char *read_line();
int mygetc(FILE * f) {
  static char *p;
  char ch;
  if (!isatty(0)) {
    // stdin is not a tty. Call real getc
    return getc(f);
  }
  // stdin is a tty. Call our read_line.
  if (p==NULL || *p == 0) {
    char * s = read_line();
    p = s;
  }
  ch = *p;
  p++;
  return ch;
}
#undef getc
#define getc(f) mygetc(f)
/* End added code for the line editory */

static  void yyunput (int c,char *buf_ptr  );

void myunputc(int c) {
    unput(c);
}

%}

%%

\n {
  return NEWLINE;
}

[ \t] {
    /* Discard spaces and tabs */
}

">" {
  return GREAT;
}

">>" { 
  return GREATGREAT;
}

">&" {
  return GREATAMPERSAND;
}

">>&" {
  return GREATGREATAMPERSAND;
}
"2>" {
  return GREATAMPERSAND;
}

"2>&1" {
  return ERRTOOUT;
}


"<" {
  return LESS;
}

"|" { 
  return PIPE;
}

"&" { 
  return AMPERSAND;
}

[\`][^\n\`]*[\`] {
  // Handles subshell commands using proccesses and pipes.
   yytext++;
   yytext[strlen(yytext)-1] = '\0';
   char *subCmd = strdup(yytext);

   int child_pipe[2];
   int parent_pipe[2];
   pipe(child_pipe);
   pipe(parent_pipe);

   int ret = fork();
   if (ret == 0) {
      // Set stdin to child pipe in
      dup2(child_pipe[0],0);
      // Set stdout to parent pipe out
      dup2(parent_pipe[1],1);

      // Close these because we made a copy
      close(child_pipe[0]);
      close(parent_pipe[1]);
      // Close these to to read from their other sides
      close(child_pipe[1]);
      close(parent_pipe[0]);


      char **arg = (char **)malloc(sizeof(char *)*2);
      arg[0] = (char *)Command::_currentCommand.shellPath;
      arg[1] = NULL;
      execvp(arg[0], arg) ;
      exit(1);
   }

   else if (ret < 0) {
     perror("fork");
     _exit(0);
   }

   write(child_pipe[1], subCmd, strlen(subCmd));
   write(child_pipe[1], "\nexit\n",1);

   // Both sides of child is done
   close(child_pipe[0]);
   close(child_pipe[1]);
   // Close this side so that the other side can be read from
   close(parent_pipe[1]);

   // I set child pipe 0 to input. The input was read in from the write call above

   char *output = (char *)malloc(sizeof(char)*100);
   int output_size = 100;
   char ch;
   int i = 0;
   // Read in the results to output
   int read_flag = 1;
   while (read_flag > 0) {
      if (i >= output_size) {
         output = (char *)realloc(output,sizeof(char)*2*output_size);
         output_size *= 2;
      }
      read_flag = read(parent_pipe[0], &output[i++], 1);
   }

   // Close final pipe
   close(parent_pipe[0]);
   
   // The input adds an eol and \n char
   output[i-2] = '\0';

   // put the subshell's output back into the parent shells input buffer
   for (i = strlen(output); i >= 0; i--) {
      if (output[i] == '\n')
         unput(' ');
      else
         unput(output[i]);
   }
   free(subCmd);
   free(output); 
}


\"[^\n\"]*\" {
  yytext[strlen(yytext)-1] = '\0';
  yylval.string_val = strdup(++yytext);
  return WORD;
}

[^ ><|&\t\n]*\\[^ \t\n]+ { 
  char *temp = (char *)malloc(sizeof(char)*strlen(yytext));
  int i;
  int j = 0;
  for (i = 0; i < strlen(yytext); i++) {
    if (yytext[i] == '\\')
      temp[j++] = yytext[++i];
    else
      temp[j++] = yytext[i];
  }
  temp[j] = '\0';

  yylval.string_val = strdup(temp);
  free(temp);
  return WORD;
}
[^ \\><|&\t\n][^ \\><|&\t\n]* {
  yylval.string_val = strdup(yytext);
  return WORD;
}



