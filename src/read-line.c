/*
 * CS354: Operating Systems Example
 * Purdue University
 * Extended by Tanner McRae
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#define MAX_BUFFER_LINE 2048

// char *read_line();

void read_line_print_usage()
{
  char * usage = (char *)"\n"
    " ctrl-?       Print usage\n"
    " Backspace    Deletes last character\n"
    " up arrow     See last command in the history\n";

  write(1, usage, strlen(usage));
}

// Buffer where line is stored
int line_length;;
int line_offset;
char line_buffer[MAX_BUFFER_LINE];
int history_tracker = -1;
int history_length = 0;
char *history[100];

char *read_line();
void add_history(char *);
char *get_next_history();
char *get_prev_history();

/* 
 * Input a line with some basic editing.
 */
char * read_line() {
// Set terminal in raw mode
tty_raw_mode();

line_length = 0;
line_offset = 0;

// Read one line until enter is typed
while (1) {

  // Read one character in raw mode.
  char ch;
  read(0, &ch, 1);

  if (ch>=32 && ch != 127) {
    // It is a printable character. 

    // Do echo

    // If max number of character reached return.
    if (line_length==MAX_BUFFER_LINE-2) break; 

    /* Begin add char to buffer */

    // If the cursor is not at the end of the word
    if (line_offset != line_length) {
      int i;
      char temp;
      for (i = line_offset; i < line_length+1; i++) {
        temp = line_buffer[i];
        write(1,&ch,1);
        line_buffer[i] = ch;
        ch = temp;
      }
      temp = 8;
      for (i=line_offset+1; i < line_length+1; i++) 
        write(1,&temp,1);
    }
    else {
      write(1,&ch,1);
      line_buffer[line_length]=ch;
    }
    line_length++;
    line_offset++;

    /* End add char to buffer */
  }
  // <Enter> was typed. Return line
  else if (ch==10) {
    // Print newline
    write(1,&ch,1);
    break;
  }
  else if (ch == 31) {
    // ctrl-?
    read_line_print_usage();
    line_buffer[0]=0;
    break;
  }
   
  // <backspace> was typed. Remove previous character read. (127 is for mac)
  else if (ch == 8 || ch == 127 && line_length>0) {
    // Go back one character
    ch = 8;
    write(1,&ch,1);

    // Write a space to erase the last character read

    if (line_offset != line_length) {
      int i;
      // Shift all characters over one. 
      for (i = line_offset; i < line_length; i++) {
        write(1,&line_buffer[i],1);
        line_buffer[i-1] = line_buffer[i];
      }
      // replace the left over part of the line w/ empty space
      ch = ' ';
      write(1,&ch,1);
      // Move cursor back to position
      ch = 8;
      for (i = line_offset; i < line_length+1; i++)
        write(1,&ch,1);
    }
    else {
      // Write empty space to get rid of character
      ch = ' ';
      write(1,&ch,1);
      // Go back one character
      ch = 8;
      write(1,&ch,1);
    }

    // Remove one character from buffer
    line_length--;
    line_offset--;
  }

  else if (ch == 4) {
    // Write a space to erase the last character read

    if (line_offset != line_length) {
      int i;
      // Shift all characters over one. 
      for (i = line_offset; i < line_length; i++) {
        write(1,&line_buffer[i+1],1);
        line_buffer[i] = line_buffer[i+1];
      }
      // replace the left over part of the line w/ empty space
      ch = ' ';
      write(1,&ch,1);
      // Move cursor back to position
      ch = 8;
      for (i = line_offset; i < line_length; i++)
        write(1,&ch,1);

      line_length--;
      // Note that offset stays the same b/c the curser is always in the same place
    }
  }


  // Home, or go to beginning of line
  else if (ch==1) {
    int i;
    ch = 8;
    for (i = 0; i < line_offset; i++)
      write(1,&ch,1);
    line_offset -= line_offset;
  }

  else if (ch==5) {
    int i;
    for (i = line_offset; i < line_length; i++)
        write(1,&line_buffer[i],1);
    line_offset = line_length;
  }

  else if (ch==27) {
    // Escape sequence. Read two chars more
    //
    // HINT: Use the program "keyboard-example" to
    // see the ascii code for the different chars typed.
    //
    char ch1; 
    char ch2;
    read(0, &ch1, 1);
    read(0, &ch2, 1);
    if (ch1==91 && ch2==65 && history_tracker > 0) {
      // fprintf(stderr, "This is index %d\n", history_index );
    // Up arrow. Print next line in history.
    // Erase old line
    // Print backspaces
      int i = 0;
      for (i =0; i < line_length; i++) {
        ch = 8;
        write(1,&ch,1);
      }

      // Print spaces on top
      for (i =0; i < line_length; i++) {
        ch = ' ';
        write(1,&ch,1);
      }

      // Print backspaces
      for (i =0; i < line_length; i++) {
        ch = 8;
        write(1,&ch,1);
      } 

      // Copy line from history
      // fprintf(stderr, "line to copy%s\n", history[history_length-1] );
      strcpy(line_buffer, get_prev_history());
      line_length = strlen(line_buffer);
      line_offset = line_length;

      // echo line
      write(1, line_buffer, line_length);
    }

    if (ch1==91 && ch2==66 && history_tracker < history_length) {
    // Down. Print prev line in history.
    // Erase old line
    // Print backspaces
      int i = 0;
      for (i =0; i < line_length; i++) {
        ch = 8;
        write(1,&ch,1);
      }

      // Print spaces on top
      for (i =0; i < line_length; i++) {
        ch = ' ';
        write(1,&ch,1);
      }

      // Print backspaces
      for (i =0; i < line_length; i++) {
        ch = 8;
        write(1,&ch,1);
      } 
        // Copy line from history
      strcpy(line_buffer, get_next_history());
      line_length = strlen(line_buffer);
      line_offset = line_length;
      write(1, line_buffer, line_length);
    }
    // Left arrow
    else if (ch1 == 91 && ch2 == 68) {
      if(line_offset > 0){
          ch = 8;
          write(1,&ch,1);
          line_offset--;
      }
    }
    // Right arrow
    else if (ch1 == 91 && ch2 == 67) {
      if (line_offset != line_length) {
        write(1,&line_buffer[line_offset],1);
        line_offset++;
      }
    }
  }
}

// Add eol and null char at the end of string
// add_history(line_buffer);
if (line_length > 1) {
  line_buffer[line_length]='\0';;
  add_history(strdup(line_buffer));
}
if (line_length == 1) {
}
line_buffer[line_length]=10;
line_length++;
line_buffer[line_length]=0;
history_tracker = history_length;
tty_can_mode();
return line_buffer;
}

void add_history(char *hist_sentence) {
  history[history_length++] = hist_sentence;
}

char * get_prev_history() {
  if (history_tracker != 0)
    return history[--history_tracker];
else 
  return history[history_tracker];
}

char *get_next_history() {
  if (history_tracker < history_length-1)
    return history[++history_tracker];
  else {
    history_tracker++;
    return (char *)"";
  }
}


