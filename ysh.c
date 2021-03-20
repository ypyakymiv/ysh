#include <stdlib.h>
#include <stdio.h>
#include "utils.h"
#include "ysh_utils.h"

int main(int argc, char **argv) {
  struct buffer *input = ec_malloc(sizeof(struct buffer));
  init_buffer(input);
  struct command *input_command;

  do {
    write_prompt();
    read_line(input);
    parse(input->data, &input_command);
    exec_command(input_command);
  } while(0);
}
