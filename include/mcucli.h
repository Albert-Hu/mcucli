#ifndef _MCUCLI_H_
#define _MCUCLI_H_

#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

#include "config.h"

#define UNUSED(x) (void)(x)

#define MCUCLI_COMMAND_OK 0
#define MCUCLI_COMMAND_NOT_FOUND 1
#define MCUCLI_COMMAND_NO_ENOUGH_BUFFER 2
#define MCUCLI_COMMAND_TOO_MANY_ARGUMENTS 3

#define MCUCLI_STATE_NORMAL 0
#define MCUCLI_STATE_ESC 1
#define MCUCLI_STATE_ESC_BRACKET 2
#define MCUCLI_STATE_DELETE 3
#define MCUCLI_STATE_ENTER 4

typedef int (*byte_writer_t)(char byte);
typedef void (*command_handler_t)(void *user_data, int argc, char *argv[]);
typedef void (*unknown_command_handler_t)(void *user_data, const char *command);
typedef struct _mcucli_command {
  const char *name;
  const char *help;
  command_handler_t handler;
} mcucli_command_t;

typedef struct _mcucli {
  uint8_t state;
  uint8_t prev_char;
  size_t len;
  size_t cursor;
  size_t num_commands;
  char line[CONFIG_MAX_BUFFER_SIZE];
  char *arguments[CONFIG_MAX_ARGUMENTS];
  mcucli_command_t *commands;
  byte_writer_t writer;
  unknown_command_handler_t unknown_command_handler;
  void *user_data;
} mcucli_t;

void mcucli_init(mcucli_t *cli, mcucli_command_t *commands, size_t num_commands,
                 byte_writer_t writer,
                 unknown_command_handler_t unknown_command_handler,
                 void *user_data);

uint8_t mcucli_push_char(mcucli_t *cli, char c);

uint8_t mcucli_command_execute(mcucli_command_t *commands, size_t num_commands,
                               const char *line, void *user_data);

#endif // _MCUCLI_H_
