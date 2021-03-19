#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "utils.h"
#include "ysh_utils.h"

#define SHELL_PROMPT "ysh>"

// just writes the prompt

void write_prompt() {
  printf(SHELL_PROMPT);  
}

// reads input and resizes buffer by the line

void read_line(struct buffer *b) {
  b->read = getline(&b->data, &b->sz, stdin);  //create an error checked function
}

// initializes buffer

void init_buffer(struct buffer *b) {
  b->data = ec_malloc(sizeof(char) * READ_BUFFER_SZ);
  b->sz = READ_BUFFER_SZ;
  b->read = 0;
}

void init_command(struct command *c) {
  c->next = NULL;
  c->name = NULL;
  c->args = NULL;
  c->in = STDIN_FILENO;
  c->out = STDOUT_FILENO;
  c->flags = 0x0;
}

void init_command_node(struct command_node *cn) {
  cn->next = NULL;
  cn->curr = NULL;
}

int parse(char *input_text, struct command **output) {
  struct command *cmd = ec_malloc(sizeof(struct command));
  struct command *curr = cmd;
  struct command *new;
  init_command(cmd);
  cmd->name = ec_malloc(sizeof(char) * (strlen(input_text) + 1));
  strcpy(cmd->name, input_text);

  char *token;

  // first parse for & operator
  char *async_op = "&";
  token = strtok(cmd->name, async_op);
  new = ec_malloc(sizeof(struct command));
  init_command(new);
  new->name = token;
  while((token = strtok(NULL, async_op))) {
    new = ec_malloc(sizeof(struct command));
    init_command(new);
    new->name = token;
    curr->flags = curr->flags|ASYNC;
    curr->next = new;
    curr = new;
    token = strtok(NULL, async_op);
  }

  // then parse for pipe operators |
  curr = cmd;
  char *pipe_op = "|";
  while(curr) {
    token = strtok(curr->name, pipe_op);
    curr->name = token;
    while((token = strtok(NULL, pipe_op))) {
      new = ec_malloc(sizeof(struct command));
      init_command(new);
      new->name = token;
      curr->flags |= PIPE_INTO;
      curr->next = new;
      curr = new;
    }
    curr = curr->next;
  }

  // then parse for redirect operators <, >, >>
  curr = cmd;
  char *read_op = "<";
  while(curr) {
    token = strtok(curr->name, read_op);
    curr->name = token;
    if((token = strtok(NULL, read_op))) {
      // read in from token
      // open fd and insert into in
      curr->in = ec_open(token, O_RDONLY);
    }
    curr = curr->next;
  }

  curr = cmd;
  char *append_op = ">>";
  while(curr) {
    token = strtok(curr->name, append_op);
    curr->name = token;
    if((token = strtok(NULL, append_op))) {
      // append to token
      // open fd and insert into out
      // set append
      curr->out = ec_open(token, O_WRONLY|O_APPEND|O_CREAT);
    }
    curr = curr->next;
  }
  
  curr = cmd;
  char *write_op = ">";
  while(curr) {
    token = strtok(curr->name, write_op);
    curr->name = token;
    if((token = strtok(NULL, write_op))) {
      // write to token
      // open fd and insert into out
      curr->out = ec_open(token, O_WRONLY|O_APPEND|O_CREAT);
    }
    curr = curr->next;
  }

  // then parse for arguments
  curr = cmd;
  char *arg_sep = " ";
  while(curr) {
    token = strtok(curr->name, arg_sep);
    curr->name = token;
    while((token = strtok(NULL, arg_sep))) {
      printf("%s", token);      
    }
    curr = curr->next;
  }
  return 0;
}
