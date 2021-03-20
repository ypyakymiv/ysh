#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int main() {
  printf("%d\n", open("testfile", O_RDONLY));
}

