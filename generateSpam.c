#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char** argv) {
  int numBytes;
  char* fileName;
  int i;
  char ch;
  FILE* fp;

  if (argc < 3) {
    printf("Usage: > ./generateSpam <fileName> <size of file>\n");
    exit(1);
  }

  fileName = argv[1];
  numBytes = atoi(argv[2]);
  if (numBytes <= 0) { 
    printf("Please specify a file size GREATER than 0. thx\n");
    exit(1);
  }

  fp = fopen(fileName, "w");
  if (!fp) {
    printf("Unable to open file %s for writing.  Exiting...\n", fileName);
    exit(1);
  }

  for (i = 0; i < numBytes; i++) {
    ch = '0' + (rand() % ('z' - '0'));
    fputc(ch, fp);
  }

  fclose(fp);
  printf("File %s written to %d bytes.  Enjoy!\n", fileName, numBytes);

  return 0;
}
