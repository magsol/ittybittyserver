#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char** argv) {
  char buf[100];
  buf[0] = 'a';
  buf[1] = 'b';

  memcpy(buf, NULL, 0);
  printf("%s\n", buf);

  return 0;
}
