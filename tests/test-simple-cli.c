#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "mcucli.h"

static void mcucli_exit(mcucli_command_t *command, void *user_data, int argc, char *argv[]);
static void mcucli_echo(mcucli_command_t *command, void *user_data, int argc, char *argv[]);
static void mcucli_version(mcucli_command_t *command, void *user_data, int argc, char *argv[]);
static void mcucli_help(mcucli_command_t *command, void *user_data, int argc, char *argv[]);

static mcucli_command_t commands[] = {
    {"exit", "Exit the program.", mcucli_exit},
    {"echo", "Echo the user input.", mcucli_echo},
    {"version", "Show the version.", mcucli_version},
    {"help", "Show the usage.", mcucli_help}};
static const size_t num_commands = sizeof(commands) / sizeof(commands[0]);

static void mcucli_unknown_command(void *user_data, const char *command) {
  UNUSED(user_data);

  printf("Unknown command: %s\r\n", command);
}

static void mcucli_exit(mcucli_command_t *command, void *user_data, int argc, char *argv[]) {
  int *stop = (int *)user_data;

  UNUSED(command);
  UNUSED(argc);
  UNUSED(argv);

  *stop = 1;
}

static void mcucli_echo(mcucli_command_t *command, void *user_data, int argc, char *argv[]) {
  UNUSED(command);
  UNUSED(user_data);
  for (int i = 0; i < argc; i++) {
    printf("%s\r\n", argv[i]);
  }
  printf("\r\n");
}

static void mcucli_version(mcucli_command_t *command, void *user_data, int argc, char *argv[]) {
  UNUSED(command);
  UNUSED(user_data);
  UNUSED(argc);
  UNUSED(argv);

  printf("1.0.0\r\n");
}

static void mcucli_help(mcucli_command_t *command, void *user_data, int argc, char *argv[]) {
  UNUSED(command);
  UNUSED(user_data);
  UNUSED(argc);
  UNUSED(argv);

  for (size_t i = 0; i < num_commands; i++) {
    printf("%s: %s\r\n", commands[i].name, commands[i].help);
  }
}

static int stdio_write(char byte) { return write(STDOUT_FILENO, &byte, 1); }

int main() {
  char c;
  int stop;
  mcucli_t cli;
  struct termios orig;
  struct termios raw;

  tcgetattr(STDIN_FILENO, &orig);

  raw = orig;
  raw.c_lflag &= ~(ECHO | ICANON);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

  mcucli_init(&cli, commands, num_commands, stdio_write, mcucli_unknown_command,
              &stop);

  while (read(STDIN_FILENO, &c, 1) == 1) {
    mcucli_push_char(&cli, c);
    if (stop) {
      break;
    }
  }

  tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig);

  return 0;
}