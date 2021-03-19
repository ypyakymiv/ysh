#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include "utils.h"

void *ec_malloc(size_t sz) {
  void *new_ptr = malloc(sz);
  if(!new_ptr) {
    perr_and_exit(ERROR_EXIT_CODE);
  }
  return new_ptr;
}

FILE *ec_fopen(const char *filename, const char *mode) {
  FILE *f = fopen(filename, mode);
  if(!f) {
    perr_and_exit(ERROR_EXIT_CODE);
  }
  return f;
}

void perr_and_exit(int code) {
  fprintf(stderr, "%s\n", strerror(errno));
  exit(code);
}

int ec_fseek_backwards(FILE *stream, long offset, int whence) {
  long curr = ftell(stream);
  if(curr < offset) { 
    fseek(stream, 0x0, SEEK_SET);
    return curr;
  } else if(fseek(stream, offset, whence)) {
    if(errno == EINVAL) {
      fseek(stream, 0x0, SEEK_SET);
    }

     // handle error
    
  } 
  return offset;
}

int is_same_file(char *a, char *b) {
  char actualpath_a[PATH_MAX + 1];
  char actualpath_b[PATH_MAX + 1];
  realpath(a, actualpath_a);
  realpath(b, actualpath_b);
  if(strcmp(actualpath_a, actualpath_b) == 0) {
    return 1; 
  } else return 0;
}

int ec_open(const char *filename, int flags) {
  return open(filename, flags);  
}
