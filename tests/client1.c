#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include "../headers/memList.h"

int main(int argc, char** argv) {
  memnode* list = getMemList(1);
  list->proxyState = BUSY;

  return 0;
}
