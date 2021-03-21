#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <signal.h>
#include <sys/wait.h>
#include "utils.h"
#include "ysh_utils.h"

#define START_ARGS 10
#define SHELL_PROMPT "ysh>"

void append_with_space(char *, char *);
void erase_newline(char *);
char *trim_ws(char *, char **);
int internal_command(struct command *);
int dir_exists(char *);

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
  char *filename;

  // first parse for & operator
  char *async_op = "&";
  token = strtok(cmd->name, async_op);
  curr->name = token;
  if(strcmp(curr->name, input_text) != 0) // & was found
    curr->flags |= ASYNC;
  while((token = strtok(NULL, async_op))) {
    new = ec_malloc(sizeof(struct command));
    init_command(new);
    new->name = token;
    curr->flags |= ASYNC;
    curr->next = new;
    curr = new;
  }

  // then parse for pipe operators |
  curr = cmd;
  char *pipe_op = "|";
  while(curr) {
    token = strtok(curr->name, pipe_op);
    curr->name = token;
    while((token = strtok(NULL, pipe_op))) {
      int p[2];
      pipe(p);
      new = ec_malloc(sizeof(struct command));
      init_command(new);
      new->flags = curr->flags;
      new->name = token;
      new->in = p[0];
      curr->out = p[1];
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
      char *end;
      filename = trim_ws(token, &end);
      curr->in = ec_open(filename, O_RDONLY);
      if(*end)
        append_with_space(curr->name, end);
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
      char *end;
      filename = trim_ws(token, &end);
      curr->out = ec_open(filename, O_WRONLY|O_APPEND|O_CREAT);
      if(*end) append_with_space(curr->name, end);
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
      char *end;
      filename = trim_ws(token, &end);
      curr->out = ec_open(filename, O_WRONLY|O_APPEND|O_CREAT);
      if(*end) append_with_space(curr->name, end);
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

void append_with_space(char *to, char *from) {
  strcat(to, " ");
  strcat(to, from);
}

int exec_command(struct command *cmd) {
  // set the pipe fd
  // discover async
  struct command *curr = cmd;
  while(curr) {
    int pid = fork();
    if(pid) {
      // should we wait
      int outcome;
      if(!curr->flags&ASYNC)
        waitpid(pid, &outcome, 0x0);
      if(curr->in != STDIN_FILENO) close(curr->in);
      if(curr->out != STDOUT_FILENO) close(curr->out);
    } else {
      if(curr->next && curr->next->in != STDIN_FILENO)
        close(curr->next->in);

      // in the child
      if(curr->in != STDIN_FILENO) {
        dup2(curr->in, STDIN_FILENO);
        close(curr->in);
      }
      if(curr->out != STDOUT_FILENO) {
        dup2(curr->out, STDOUT_FILENO);
        close(curr->out);
      }
      if(internal_command(curr))
        exit(0);
      else
        execvp(*curr->args, curr->args);
    }
    curr = curr->next;
  }
  return 0;
}

void erase_newline(char *a) {
  while(*a) {
    if(*a == '\n') *a = '\0';
    a++;
  }
}

char *trim_ws(char *a, char **end) {
  while(*a && *a == ' ') a++;
  char *start = a;
  while(*a && *a != ' ') a++;
  if(*a) *end = a + 1;
  else *end = a;
  *a = '\0';
  return start;
}

void echo(struct command *cmd) {
  char **args1 = cmd->args + 1;
  if(*args1) {
    printf("%s", *args1);
    args1++;
  }
  while(*args1) {
   printf(" %s", *args1);
   args1++;
  }
}

int cd(struct command *cmd) {
  if(!cmd->args[1]) return 1;
  if(dir_exists(cmd->args[1])) {
    // change working dir
    char *new_path = realpath(cmd->args[1], NULL);
    // set path
    setenv("PWD", new_path, 1);
    chdir(new_path);
    // free path
    free(new_path);
  } else return 0;
}

void clr() {
  // print many newlines
  const int magic_amount_of_newlines = 100;
  for(int i = 0; i < magic_amount_of_newlines; i++)
    printf("\n");
}

void dir(struct command *cmd) {
  // if no first arg show cur dir
  char *dir_to_scan;
  if(cmd->args[1]) {
    dir_to_scan = cmd->args[1];
  } else {
    dir_to_scan = ".";
  }
  if(dir_exists(dir_to_scan)) {
    struct dirent **list;
    int n;
    n = scandir(dir_to_scan, &list, NULL, alphasort);
    for(int i = 0; i < n; i++) {
      printf("%s\n", list[i]->d_name);
      free(list[i]);
    }
  } else {
    printf("dir %s does not exists\n", dir_to_scan);
  }

}

void i_environ() {
  char **env_var = environ;
  while(env_var) {
    printf("%s\n", *env_var);
    env_var++;
  }
}

void help() {
  printf("%-10s - %s\n", "help", "display help information");
  printf("%-10s - %s\n", "environ", "display all environ variables");
  printf("%-10s - %s\n", "echo <...>", "echo all args to output");
  printf("%-10s - %s\n", "cd <dir>", "change directory");
  printf("%-10s - %s\n", "pause", "pause the shell until enter");
  printf("%-10s - %s\n", "clr", "clear the screen");
  printf("%-10s - %s\n", "dir <dir>", "display directory contents");
  printf("%-10s - %s\n", "quit", "quit the shell");
  printf("you may run any executable in PATH\n");
  printf("the & operator appended to any command makes it run in background\n");
  printf("the | operator can be used in between commands to feed the output of one command to the input of another\n");
  printf("< <filename> redirects input in from a file\n");
  printf("> <filename> redirects output to a file\n");
  printf(">> <filename> redirects output and appends to a file\n");
}

void i_pause() {
  // wait until enter
}

void quit() {
  int ppid = getppid();
  kill(ppid, SIGUSR2);
}

int internal_command(struct command *cmd) {
  const char *i_cmds[] = {"echo", "cd", "clr", "dir", "environ", "help", "pause", "quit", NULL};
  enum i_cmds_index {ECHO, CD, CLR, DIR, ENVIRON, HELP, PAUSE, QUIT};
  int match = -1;


  int i = 0;
  while(i_cmds[i]) {
    if(strcmp(i_cmds[i], *cmd->args) == 0) {
      match = i;
      break;
    }
    i++;
  }

  switch(match) {
    case ECHO:
      echo(cmd);
      break;
    case CD:
      cd(cmd);
      break;
    case CLR:
      clr();
      break;
    case DIR:
      dir(cmd);
      break;
    case ENVIRON:
      i_environ();
      break;
    case HELP:
      help();
      break;
    case PAUSE:
      i_pause();
      break;
    case QUIT:
      quit();
      break;
    default:
      return 0;
      break;
  }
  return 1;
}

int dir_exists(char *path) {
  if(realpath(path, NULL))
    return 1;
  else return 0;
}
