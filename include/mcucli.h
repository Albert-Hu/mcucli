#ifndef _MCUCLI_H_
#define _MCUCLI_H_

#include <stddef.h>
#include <stdint.h>

#define UNUSED(x) (void)(x)

#define MCUCLI_COMMAND_OK 0
#define MCUCLI_COMMAND_NOT_FOUND 1
#define MCUCLI_COMMAND_NO_ENOUGH_BUFFER 2
#define MCUCLI_COMMAND_TOO_MANY_ARGUMENTS 3

typedef void (*command_handler_t)(int argc, char *argv[]);

typedef struct _mcucli_command_t {
  const char *name;
  const char *help;
  command_handler_t handler;
} mcucli_command_t;

uint8_t mcucli_command_execute(mcucli_command_t *commands, size_t num_commands,
                               const char *line);

#endif // _MCUCLI_H_
