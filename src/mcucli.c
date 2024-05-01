#include <string.h>

#include "mcucli.h"

static void mcucli_execute(mcucli_t *cli);
static void mcucli_reset(mcucli_t *cli);
static void mcucli_input_normal(mcucli_t *cli, void *user_data, char c);
static void mcucli_input_esc(mcucli_t *cli, void *user_data, char c);
static void mcucli_input_esc_seq1(mcucli_t *cli, void *user_data, char c);
static void mcucli_input_esc_seq2(mcucli_t *cli, void *user_data, char c);
static void mcucli_input_esc_seq3(mcucli_t *cli, void *user_data, char c);
static void mcucli_input_esc_seq4(mcucli_t *cli, void *user_data, char c);
static void mcucli_input_delete(mcucli_t *cli, void *user_data, char c);
static void mcucli_insert_character(mcucli_t *cli, char c);
static void mcucli_remove_character(mcucli_t *cli, uint8_t is_backspace);

static int mcucli_bytes_write(const char *bytes, size_t len) {
  UNUSED(bytes);
  return len;
}

static void mcucli_unknown_command_handler(mcucli_t *cli, void *user_data,
                                           const char *command) {
  UNUSED(cli);
  UNUSED(user_data);
  UNUSED(command);
}

static void mcucli_reset(mcucli_t *cli) {
  cli->len = 0;
  cli->cursor = 0;
}

static void mcucli_remove_character(mcucli_t *cli, uint8_t is_backspace) {
  if ((is_backspace && cli->cursor == 0) ||
      (!is_backspace && cli->cursor == cli->len)) {
    return;
  }

  for (size_t i = cli->cursor - is_backspace; i < cli->len; i++) {
    cli->buffer.line[i] = cli->buffer.line[i + 1];
  }

  cli->len--;

  if (is_backspace) {
    cli->cursor--;
    cli->write("\b", 1);
  }

  cli->write(cli->buffer.line + cli->cursor, cli->len - cli->cursor);
  cli->write(" ", 1);

  for (size_t i = cli->len - cli->cursor + 1; i > 0; i--) {
    cli->write("\b", 1);
  }
}

static void mcucli_insert_character(mcucli_t *cli, char c) {
  if (cli->len < cli->buffer.line_size - 1 && c >= 0x20 && c <= 0x7E) {
    for (size_t i = cli->len; i > cli->cursor; i--) {
      cli->buffer.line[i] = cli->buffer.line[i - 1];
    }
    cli->buffer.line[cli->cursor] = c;
    cli->len++;
    cli->cursor++;
    cli->write(cli->buffer.line + cli->cursor - 1, cli->len - cli->cursor + 1);
    for (size_t i = cli->len - cli->cursor; i > 0; i--) {
      cli->write("\b", 1);
    }
  }
}

static void mcucli_input_delete(mcucli_t *cli, void *user_data, char c) {
  UNUSED(user_data);

  if (c == '~') {
    mcucli_remove_character(cli, 0);
  }
  cli->process = mcucli_input_normal;
}

static void mcucli_input_esc_seq4(mcucli_t *cli, void *user_data, char c) {
  UNUSED(user_data);
  UNUSED(c);

  cli->process = mcucli_input_normal;
}

static void mcucli_input_esc_seq3(mcucli_t *cli, void *user_data, char c) {
  UNUSED(user_data);

  cli->process =
      (c == '0' || c == '1') ? mcucli_input_esc_seq4 : mcucli_input_normal;
}

static void mcucli_input_esc_seq2(mcucli_t *cli, void *user_data, char c) {
  UNUSED(user_data);

  if (c == '~') {
    while (cli->cursor > 0) {
      cli->cursor--;
      cli->write("\033[D", 3);
    }
    cli->process = mcucli_input_normal;
  } else {
    cli->process = (c == '0' && cli->prev_char == '2') ? mcucli_input_esc_seq3
                                                       : mcucli_input_normal;
  }
}

static void mcucli_input_esc_seq1(mcucli_t *cli, void *user_data, char c) {
  UNUSED(user_data);

  cli->process = mcucli_input_normal;

  switch (c) {
  case 'A': // up
  case 'B': // down
    break;
  case 'C': // right
    if (cli->cursor < cli->len) {
      cli->cursor++;
      cli->write("\033[C", 3);
    }
    break;
  case 'D': // left
    if (cli->cursor > 0) {
      cli->cursor--;
      cli->write("\033[D", 3);
    }
    break;
  case 'O':
    cli->process = mcucli_input_esc_seq2;
    break;
  case 'F': // end
    while (cli->cursor < cli->len) {
      cli->cursor++;
      cli->write("\033[C", 3);
    }
    break;
  case 'H': // home
    while (cli->cursor > 0) {
      cli->cursor--;
      cli->write("\033[D", 3);
    }
    break;
  case '3': // delete
    cli->process = mcucli_input_delete;
    break;
  case '1':
  case '2':
  case '4':
  case '7':
  case '8':
    cli->process = mcucli_input_esc_seq2;
    break;
  default:
    break;
  }
}

static void mcucli_input_esc(mcucli_t *cli, void *user_data, char c) {
  UNUSED(user_data);

  if (c == '[' || c == 'O') {
    cli->process = mcucli_input_esc_seq1;
  } else {
    cli->process = mcucli_input_normal;
  }
}

static void mcucli_input_normal(mcucli_t *cli, void *user_data, char c) {
  UNUSED(user_data);

  if (c == 0x1B) { // ESC
    cli->process = mcucli_input_esc;
  } else if (c == 0x7F || c == 0x08) { // backspace
    mcucli_remove_character(cli, 1);
  } else if (c == '\n' || c == '\r') {
    if (cli->len > 0) {
      cli->write("\r\n", 2);
      cli->buffer.line[cli->len] = '\0';
      mcucli_execute(cli);
      mcucli_reset(cli);
    }

    cli->buffer.line[0] = c;
    if (cli->buffer.line[0] != '\r') {
      cli->write(cli->prefix, strlen(cli->prefix));
    }
  } else if (c == 0x03) {
    // handle ctrl-c event
    mcucli_reset(cli);
    cli->write("^C\r\n", 2);
    cli->write(cli->prefix, strlen(cli->prefix));
  } else {
    mcucli_insert_character(cli, c);
  }
}

static void mcucli_execute(mcucli_t *cli) {
  uint8_t argc = 0;
  size_t length = strlen(cli->buffer.line);
  char *command = cli->buffer.line;
  command_handler_t handler = NULL;

  // parse arguments
  for (size_t i = 0; i < length && cli->buffer.line[i] != '\0'; i++) {
    if (cli->buffer.line[i] == ' ') {
      cli->buffer.line[i] = '\0';
      cli->buffer.argument[argc] = cli->buffer.line + i + 1;
      argc += (cli->buffer.argument[argc][0] != '\0');
    }
  }

  // execute command
  for (size_t i = 0; i < cli->command_set.num_commands; i++) {
    if (strcmp(command, cli->command_set.commands[i].name) == 0) {
      handler = cli->command_set.commands[i].handler;
    }
  }

  if (handler == NULL) {
    cli->unknown_command_handler(cli, cli->user_data, command);
  } else {
    handler(cli, cli->user_data, argc, cli->buffer.argument);
  }

  mcucli_reset(cli);
}

void mcucli_putc(mcucli_t *cli, char c) {
  cli->process(cli, cli->user_data, c);
  cli->prev_char = c;
}

void mcucli_set_prefix(mcucli_t *cli, char *prefix) {
  cli->prefix = prefix;
  cli->write(prefix, strlen(prefix));
}

void mcucli_set_stream_handler(mcucli_t *cli, input_handler_t handler) {
  cli->process = handler;
}

void mcucli_unset_stream_handler(mcucli_t *cli) {
  cli->process = mcucli_input_normal;
  cli->write("\r\n", 2);
  cli->write(cli->prefix, strlen(cli->prefix));
  mcucli_reset(cli);
}

void mcucli_init(mcucli_t *cli, void *user_data, mcucli_buffer_t *b,
                 mcucli_command_set_t *s, bytes_write_t w,
                 unknown_command_handler_t u) {
  cli->len = 0;
  cli->cursor = 0;
  cli->buffer.line = b->line;
  cli->buffer.line_size = b->line_size;
  cli->buffer.argument = b->argument;
  cli->buffer.argument_size = b->argument_size;
  cli->command_set.commands = s->commands;
  cli->command_set.num_commands = s->num_commands;
  cli->process = mcucli_input_normal;
  cli->write = (w == NULL ? mcucli_bytes_write : w);
  cli->unknown_command_handler =
      (u == NULL ? mcucli_unknown_command_handler : u);
  cli->user_data = user_data;
  cli->prefix = "";
}
