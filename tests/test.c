#include <stdio.h>
#include <stdlib.h>
#include "memList.h"

int main(int argc, char** argv) {
  memnode* list = getMemList(10);

  if (pthread_mutex_unlock(&(list[0].mutex)) != 0) {
    printf("Error unlocking mutex!\n");
  } else {
    printf("Mutex successfully unlocked.\n");
  }

  return 0;
}
