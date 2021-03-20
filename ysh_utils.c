#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "utils.h"
#include "ysh_utils.h"

#define START_ARGS 10
#define SHELL_PROMPT "ysh>"

void append(char *, char *);
void append_from_space(char *, char *);
void erase_newline(char *);

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
  erase_newline(cmd->name);
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
      append_from_space(curr->name, token);
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
      append_from_space(curr->name, token);
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
      append_from_space(curr->name, token);
      curr->out = ec_open(token, O_WRONLY|O_APPEND|O_CREAT);
    }
    curr = curr->next;
  }

  // then parse for arguments
  curr = cmd;
  char *arg_sep = " ";
  while(curr) {
    char **arg_list = ec_malloc(START_ARGS * sizeof(char *));
    int len = START_ARGS;
    int curr_sz = 0;
    memset(arg_list, 0x0, len * sizeof(char *));
    token = strtok(curr->name, arg_sep);
    int counter = 0;
    do {
      if(*token) { //check if non empty
        arg_list[curr_sz] = token;
        curr_sz++;
      }
    } while((token = strtok(NULL, arg_sep)));
    arg_list[curr_sz] = NULL;
    curr->args = arg_list;
    curr = curr->next;
  }
  *output = cmd;
  return 0;
}

void append(char *to, char *from) {
  while(*to) to++;
  while(*from) {
    *to = *from;
    to++;
    from++;
  }

  *to = '\0';  
}

void append_from_space(char *to, char *from) {
  while(*from && *from == ' ') from++; // iterate into next word
  while(*from && *from != ' ') from++; // iterate to next space
  append(to, from);
}

int exec_command(struct command *cmd) {
  // set the pipe fd
  // discover async

  int pid = fork();
  if(pid) {
    // should we wait
    int outcome;
    waitpid(pid, &outcome, 0x0);
  } else {
    // in the child
    
    execvp(*cmd->args, cmd->args);
  }
  
  return 0;
}

void erase_newline(char *a) {
  while(*a) {
    if(*a == '\n') *a = '\0';
    a++;
  }
}
