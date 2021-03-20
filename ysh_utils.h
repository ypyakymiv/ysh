#ifndef YSH_UTILS_INCLUDED
#define YSH_UTILS_INCLUDED

struct buffer {
  char *data;
  size_t sz;
  int read;
};

void init_buffer(struct buffer *);
void write_prompt();
void read_line(struct buffer *);

enum command_flags {
  ASYNC = 0x1,
  PIPE_INTO = 0x2,
  APPEND = 0x4
};

struct command;
struct command {
  struct command *next;
  char *name;
  char **args;
  int in;
  int out;
  int flags;
};

void init_command(struct command *);

struct command_node;
struct command_node {
  struct command_node *next;
  struct command *curr;
};

void init_command_node(struct command_node *);

struct command ASYNC_TERM;

int parse(char *, struct command **);
int exec_command(struct command *);


#endif
