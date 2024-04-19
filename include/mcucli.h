#ifndef _MCUCLI_H_
#define _MCUCLI_H_

#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

#define UNUSED(x) (void)(x)

typedef struct _mcucli mcucli_t;
typedef struct _mcucli_buffer mcucli_buffer_t;
typedef struct _mcucli_command mcucli_command_t;
typedef struct _mcucli_command_set mcucli_command_set_t;

typedef int (*byte_writer_t)(char byte);

typedef void (*unknown_command_handler_t)(mcucli_t *cli, void *user_data, const char *command);
typedef void (*command_handler_t)(mcucli_t *cli, void *user_data, int argc, char *argv[]);
typedef void (*input_handler_t)(mcucli_t *cli, void *user_data, char c);
typedef int (*bytes_write_t)(const char *bytes, size_t len);

struct _mcucli_buffer {
  char *line;
  size_t line_size;
  char **argument;
  size_t argument_size;
};

struct _mcucli_command {
  char *name;
  char *help;
  command_handler_t handler;
};

struct _mcucli_command_set {
  size_t num_commands;
  mcucli_command_t *commands;
};

struct _mcucli {
  size_t len;
  size_t cursor;
  mcucli_buffer_t buffer;
  mcucli_command_set_t command_set;
  input_handler_t process;
  bytes_write_t write;
  unknown_command_handler_t unknown_command_handler;
  char *prefix;
  void *user_data;
};

void mcucli_init(mcucli_t *cli, void *user_data, mcucli_buffer_t *b, mcucli_command_set_t *s, bytes_write_t w, unknown_command_handler_t u);
void mcucli_set_prefix(mcucli_t *cli, char *prefix);
void mcucli_set_stream_handler(mcucli_t *cli, input_handler_t handler);
void mcucli_unset_stream_handler(mcucli_t *cli);
void mcucli_putc(mcucli_t *cli, char c);

#endif // _MCUCLI_H_
