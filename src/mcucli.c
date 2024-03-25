#include <string.h>

#include "config.h"
#include "mcucli.h"

static char mcucli_buffer[CONFIG_MAX_BUFFER_SIZE];
static char *mcucli_arguments[CONFIG_MAX_ARGUMENTS];

uint8_t mcucli_command_execute(mcucli_command_t *commands, size_t num_commands,
                               const char *line) {
  uint8_t argc = 0;
  size_t length = strlen(line);
  char *command = mcucli_buffer;

  // check if the length of the line is greater than the buffer
  if (strlen(line) > CONFIG_MAX_BUFFER_SIZE) {
    return MCUCLI_COMMAND_NO_ENOUGH_BUFFER;
  }

  // clear buffer
  memset(mcucli_buffer, 0, CONFIG_MAX_BUFFER_SIZE);

  // copy line to buffer
  strcpy(mcucli_buffer, line);

  // parse arguments
  for (size_t i = 0; i < length; i++) {
    if (mcucli_buffer[i] == ' ') {
      mcucli_buffer[i] = '\0';
      mcucli_arguments[argc] = mcucli_buffer + i + 1;
      if (mcucli_arguments[argc][0] != '\0')
        argc++;
    }
  }

  // execute command
  for (size_t i = 0; i < num_commands; i++) {
    if (strcmp(command, commands[i].name) == 0) {
      commands[i].handler(argc, mcucli_arguments);
      return MCUCLI_COMMAND_OK;
    }
  }

  return MCUCLI_COMMAND_NOT_FOUND;
}
