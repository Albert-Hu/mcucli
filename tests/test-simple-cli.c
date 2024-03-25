#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "mcucli.h"

#define STATE_NORMAL 0
#define STATE_ESC 1
#define STATE_ESC_BRACKET 2
#define STATE_DELETE 3
#define STATE_ENTER 4

static void reset_terminal_mode(struct termios *orig_termios);

static void mcucli_exit(int argc, char *argv[]);
static void mcucli_echo(int argc, char *argv[]);
static void mcucli_version(int argc, char *argv[]);
static void mcucli_help(int argc, char *argv[]);

static mcucli_command_t commands[] = {
    {"exit", "Exit the program.", mcucli_exit},
    {"echo", "Echo the user input.", mcucli_echo},
    {"version", "Show the version.", mcucli_version},
    {"help", "Show the usage.", mcucli_help},
};

static const size_t num_commands = sizeof(commands) / sizeof(commands[0]);

static const char *version = "1.0.0";

static struct termios orig_termios;

static void mcucli_exit(int argc, char *argv[]) {
  UNUSED(argc);
  UNUSED(argv);

  reset_terminal_mode(&orig_termios);

  write(STDOUT_FILENO, "\n", 1);

  exit(0);
}

static void mcucli_echo(int argc, char *argv[]) {
  for (int i = 0; i < argc; i++) {
    const char *ptr = argv[i];
    while (*ptr != '\0') {
      write(STDOUT_FILENO, ptr, 1);
      ptr++;
    }
    write(STDOUT_FILENO, " ", 1);
  }
  write(STDOUT_FILENO, "\n", 1);
}

static void mcucli_version(int argc, char *argv[]) {
  UNUSED(argc);
  UNUSED(argv);

  const char *ptr = version;
  while (*ptr != '\0') {
    write(STDOUT_FILENO, ptr, 1);
    ptr++;
  }
  write(STDOUT_FILENO, "\n", 1);
}

static void mcucli_help(int argc, char *argv[]) {
  UNUSED(argc);
  UNUSED(argv);

  for (size_t i = 0; i < num_commands; i++) {
    const char *ptr = commands[i].name;
    while (*ptr != '\0') {
      write(STDOUT_FILENO, ptr, 1);
      ptr++;
    }
    write(STDOUT_FILENO, "\t", 1);
    ptr = commands[i].help;
    while (*ptr != '\0') {
      write(STDOUT_FILENO, ptr, 1);
      ptr++;
    }
    write(STDOUT_FILENO, "\n", 1);
  }
}

static void set_raw_mode(struct termios *orig_termios) {
  struct termios raw = *orig_termios;

  raw.c_lflag &= ~(ECHO | ICANON);

  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

static void reset_terminal_mode(struct termios *orig_termios) {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, orig_termios);
}

static void remove_character(char *line, size_t *cursor, size_t *len) {
  if (*len > 0) {
    for (size_t i = *cursor; i < *len; i++) {
      line[i] = line[i + 1];
    }
    (*len)--;
    write(STDOUT_FILENO, "\b \b", 3);
  }
}

static void insert_character(char *line, size_t *cursor, size_t *len,
                             size_t max_size, char c) {
  if (*len < max_size - 1) {
    for (size_t i = *len; i > *cursor; i--) {
      line[i] = line[i - 1];
    }
    line[*cursor] = c;
    (*len)++;
    (*cursor)++; // Move the cursor forward after inserting a character
    write(STDOUT_FILENO, line + *cursor - 1, *len - *cursor + 1);
    for (size_t i = 0; i < *len - *cursor; i++) {
      write(STDOUT_FILENO, "\b", 1);
    }
  }
}

int main() {
  char c;
  char line[256];
  int state;
  size_t cursor = 0, len = 0;
  size_t max_size = sizeof(line);

  memset(line, 0, max_size);

  tcgetattr(STDIN_FILENO, &orig_termios);

  set_raw_mode(&orig_termios);

  while (read(STDIN_FILENO, &c, 1) == 1) {
    switch (state) {
    case STATE_NORMAL:
      if (c == 27) {
        state = STATE_ESC;
      } else {
        if (c == 127) {
          if (len > 0) {
            remove_character(line, &cursor, &len);
          }
        } else if (c == '\n') {
          line[len] = '\0';
          write(STDOUT_FILENO, "\n", 1);
          mcucli_command_execute(commands, num_commands, line);
          memset(line, 0, max_size);
          len = cursor = 0;
        } else {
          if (c != '\r' && len < max_size - 1) {
            insert_character(line, &cursor, &len, max_size, c);
          }
        }
      }
      break;
    case STATE_ESC:
      state = (c == '[') ? STATE_ESC_BRACKET : STATE_NORMAL;
      break;
    case STATE_ESC_BRACKET:
      if (c == 'A') {
        // up
      } else if (c == 'B') {
        // down
      } else if (c == 'C') {
        // right
        if (cursor < len) {
          cursor++;
          write(STDOUT_FILENO, "\033[C", 3);
        }
      } else if (c == 'D') {
        // left
        if (cursor > 0) {
          cursor--;
          write(STDOUT_FILENO, "\033[D", 3);
        }
      } else if (c == 'F') {
        // end
        while (cursor < len) {
          cursor++;
          write(STDOUT_FILENO, "\033[C", 3);
        }
      } else if (c == 'H') {
        // home
        while (cursor > 0) {
          cursor--;
          write(STDOUT_FILENO, "\033[D", 3);
        }
      }
      state = (c == '3') ? STATE_DELETE : STATE_NORMAL;
      break;
    case STATE_DELETE:
      if (c == '~') {
        if (cursor < len) {
          remove_character(line, &cursor, &len);
        }
      }
      state = STATE_NORMAL;
      break;
    }
  }

  reset_terminal_mode(&orig_termios);

  return 0;
}