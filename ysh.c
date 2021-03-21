#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include "utils.h"
#include "ysh_utils.h"

void exit_func();

int main(int argc, char **argv) {
  signal(SIGUSR2, exit_func);

  struct buffer *input = ec_malloc(sizeof(struct buffer));
  init_buffer(input);
  struct command *input_command;

  do {
    write_prompt();
    read_line(input);
    parse(input->data, &input_command);
    exec_command(input_command);
  } while(1);
}

void exit_func() {
  printf("thanks for using my shell\n");
  exit(0);
}
