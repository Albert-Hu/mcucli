#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "mcucli.h"

#define LINE_BUFFER_SIZE 128
#define ARGUMENT_BUFFER_SIZE 32

static void mcucli_exit(mcucli_t *cli, void *user_data, int argc, char *argv[]);
static void mcucli_echo(mcucli_t *cli, void *user_data, int argc, char *argv[]);
static void mcucli_version(mcucli_t *cli, void *user_data, int argc, char *argv[]);
static void mcucli_help(mcucli_t *cli, void *user_data, int argc, char *argv[]);

static mcucli_command_t commands[] = {
    {"exit", "Exit the program.", mcucli_exit},
    {"echo", "Echo the user input.", mcucli_echo},
    {"version", "Show the version.", mcucli_version},
    {"help", "Show the usage.", mcucli_help}};
static const size_t num_commands = sizeof(commands) / sizeof(commands[0]);

static mcucli_command_set_t command_set = {num_commands, commands};

static char line_buffer[LINE_BUFFER_SIZE];
static char *argument_buffer[ARGUMENT_BUFFER_SIZE];
static mcucli_buffer_t buffer = {line_buffer, LINE_BUFFER_SIZE, argument_buffer, ARGUMENT_BUFFER_SIZE};

static void mcucli_unknown_command(mcucli_t *cli, void *user_data, const char *command) {
  UNUSED(cli);
  UNUSED(user_data);

  printf("Unknown command: %s\r\n", command);
}

static void mcucli_exit(mcucli_t *cli, void *user_data, int argc, char *argv[]) {
  int *stop = (int *)user_data;

  UNUSED(cli);
  UNUSED(user_data);
  UNUSED(argc);
  UNUSED(argv);

  *stop = 1;
}

static void mcucli_echo(mcucli_t *cli, void *user_data, int argc, char *argv[]) {
  UNUSED(cli);
  UNUSED(user_data);
  for (int i = 0; i < argc; i++) {
    printf("%s\r\n", argv[i]);
  }
  printf("\r\n");
}

static void mcucli_version(mcucli_t *cli, void *user_data, int argc, char *argv[]) {
  UNUSED(cli);
  UNUSED(user_data);
  UNUSED(argc);
  UNUSED(argv);

  printf("1.0.0\r\n");
}

static void mcucli_help(mcucli_t *cli, void *user_data, int argc, char *argv[]) {
  UNUSED(cli);
  UNUSED(user_data);
  UNUSED(argc);
  UNUSED(argv);

  for (size_t i = 0; i < num_commands; i++) {
    printf("%s: %s\r\n", commands[i].name, commands[i].help);
  }
}

static int stdio_write(const char *bytes, size_t size) { return write(STDOUT_FILENO, bytes, size); }

int main(int argc, char *argv[]) {
  char c;
  int stop;
  mcucli_t cli;
  struct termios orig;
  struct termios raw;

  UNUSED(argc);
  UNUSED(argv);

  tcgetattr(STDIN_FILENO, &orig);

  raw = orig;
  raw.c_lflag &= ~(ECHO | ICANON);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

  mcucli_init(&cli, &stop, &buffer, &command_set, stdio_write, mcucli_unknown_command);

  while (read(STDIN_FILENO, &c, 1) == 1) {
    mcucli_putc(&cli, c);
    if (stop) {
      break;
    }
  }

  tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig);

  return 0;
}