#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include "utils.h"
#include "ysh_utils.h"

void exit_func();

int main(int argc, char **argv) {
  signal(SIGUSR2, exit_func);
  int suppress_prompt = 0;
  if(argc > 1) {
    suppress_prompt = 1;
    // open the file and dup it to input fd
    int new_input = ec_open(argv[1], O_RDONLY);
    dup2(new_input, STDIN_FILENO);
    close(new_input);
  }

  struct buffer *input = ec_malloc(sizeof(struct buffer));
  init_buffer(input);
  struct command *input_command;

  do {
    if(!suppress_prompt)
      write_prompt();
    if(read_line(input) == EOF)
      break;
    parse(input->data, &input_command);
    exec_command(input_command);
  } while(1);
}

void exit_func() {
  printf("thanks for using my shell\n");
  exit(0);
}
