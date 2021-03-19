#ifndef UTILS_INCLUDE
#define UTILS_INCLUDE 

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

#define INVALID_ARGUMENTS 1
#define ERROR_EXIT_CODE 1
#define READ_BUFFER_SZ 1024

void *ec_malloc(size_t sz);
FILE *ec_fopen(const char *filename, const char *mode);
void perr_and_exit(int code);
int ec_fseek_backwards(FILE *stream, long offset, int whence);
int is_same_file(char *a, char *b);
int ec_open(const char *filename, int flags);

#endif
