#include <string.h>

#include "config.h"
#include "mcucli.h"

static char mcucli_buffer[CONFIG_MAX_BUFFER_SIZE];
static char *mcucli_arguments[CONFIG_MAX_ARGUMENTS];

static int mcucli_default_write_byte(char byte) {
  UNUSED(byte);
  return 1;
}

static void mcucli_default_unknown_command_handler(void *user_data,
                                                   const char *command) {
  UNUSED(user_data);
  UNUSED(command);
}

static int mcucli_remove_character(mcucli_t *cli, uint8_t is_backspace) {
  if ((is_backspace && cli->cursor == 0) ||
      (!is_backspace && cli->cursor == cli->len)) {
    return 0;
  }

  for (size_t i = cli->cursor - is_backspace; i < cli->len; i++) {
    cli->line[i] = cli->line[i + 1];
  }

  cli->len--;

  if (is_backspace) {
    cli->cursor--;
    cli->writer('\b');
  }

  for (size_t i = cli->cursor; i < cli->len; i++) {
    cli->writer(cli->line[i]);
  }
  cli->writer(' ');

  for (size_t i = cli->len - cli->cursor + 1; i > 0; i--) {
    cli->writer('\b');
  }

  return 0;
}

static int mcucli_insert_character(mcucli_t *cli, char c) {
  if (cli->len < CONFIG_MAX_BUFFER_SIZE - 1) {
    for (size_t i = cli->len; i > cli->cursor; i--) {
      cli->line[i] = cli->line[i - 1];
    }
    cli->line[cli->cursor] = c;
    cli->len++;
    cli->cursor++;
    for (size_t i = cli->cursor - 1; i < cli->len; i++) {
      cli->writer(cli->line[i]);
    }
    for (size_t i = cli->len - cli->cursor; i > 0; i--) {
      cli->writer('\b');
    }
    return 0;
  }
  return -1;
}

uint8_t mcucli_command_execute(mcucli_command_t *commands, size_t num_commands,
                               const char *line, void *user_data) {
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
      commands[i].handler(user_data, argc, mcucli_arguments);
      return MCUCLI_COMMAND_OK;
    }
  }

  return MCUCLI_COMMAND_NOT_FOUND;
}

void mcucli_init(mcucli_t *cli, mcucli_command_t *commands, size_t num_commands,
                 byte_writer_t writer,
                 unknown_command_handler_t unknown_command_handler,
                 void *user_data) {
  memset(cli, 0, sizeof(mcucli_t));
  cli->state = MCUCLI_STATE_NORMAL;
  cli->writer = (writer == NULL) ? mcucli_default_write_byte : writer;
  cli->unknown_command_handler = (unknown_command_handler == NULL)
                                     ? mcucli_default_unknown_command_handler
                                     : unknown_command_handler;
  cli->num_commands = num_commands;
  cli->commands = commands;
  cli->user_data = user_data;
}

uint8_t mcucli_push_char(mcucli_t *cli, char c) {
  uint8_t result;

  switch (cli->state) {
  case MCUCLI_STATE_NORMAL:
    if (c == 0x1B) { // ESC
      cli->state = MCUCLI_STATE_ESC;
    } else {
      if (c == 0x7F) { // backspace
        mcucli_remove_character(cli, 1);
      } else if (c == '\n' || c == '\r') {
        cli->line[cli->len] = '\0';
        cli->writer('\n');
        if (cli->len > 0) {
          result = mcucli_command_execute(cli->commands, cli->num_commands,
                                          cli->line, cli->user_data);
          if (result == MCUCLI_COMMAND_NOT_FOUND) {
            cli->unknown_command_handler(cli->user_data, cli->line);
          }
        }
        memset(cli->line, 0, CONFIG_MAX_BUFFER_SIZE);
        cli->len = cli->cursor = 0;
      } else {
        mcucli_insert_character(cli, c);
      }
    }
    break;
  case MCUCLI_STATE_ESC:
    cli->state = (c == '[') ? MCUCLI_STATE_ESC_BRACKET : MCUCLI_STATE_NORMAL;
    break;
  case MCUCLI_STATE_ESC_BRACKET:
    if (c == 'A') {
      // up
    } else if (c == 'B') {
      // down
    } else if (c == 'C') {
      // right
      if (cli->cursor < cli->len) {
        cli->cursor++;
        cli->writer('\033');
        cli->writer('[');
        cli->writer('C');
      }
    } else if (c == 'D') {
      // left
      if (cli->cursor > 0) {
        cli->cursor--;
        cli->writer('\033');
        cli->writer('[');
        cli->writer('D');
      }
    } else if (c == 'F') {
      // end
      while (cli->cursor < cli->len) {
        cli->cursor++;
        cli->writer('\033');
        cli->writer('[');
        cli->writer('C');
      }
    } else if (c == 'H') {
      // home
      while (cli->cursor > 0) {
        cli->cursor--;
        cli->writer('\033');
        cli->writer('[');
        cli->writer('D');
      }
    }
    cli->state = (c == '3') ? MCUCLI_STATE_DELETE : MCUCLI_STATE_NORMAL;
    break;
  case MCUCLI_STATE_DELETE:
    if (c == '~') {
      mcucli_remove_character(cli, 0);
    }
    cli->state = MCUCLI_STATE_NORMAL;
    break;
  }
  return cli->state;
}
