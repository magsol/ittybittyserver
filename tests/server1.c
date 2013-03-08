#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include "../headers/memList.h"

/* SERVER */

int main(int argc, char** argv) {
  int i, len;
  memnode* list = getMemList(1);
  memnode* node;
  char* msg = "This is a response. kthx";

  /*
  destroyMemList(list, 1);
  return 0;
  */

  /* wait for a request */
  printf("Waiting...\n");
  while ((node = findState(list, 1, WAITING_INIT_SRVR)) == NULL);

  /* got a request */
  list->serverState = WAITING_CONT_PRXY;

  pthread_mutex_lock(&(list->mutex));
  pthread_cond_signal(&(list->condition));
  do {
    pthread_cond_wait(&(list->condition), &(list->mutex));
    printf("%c ", list->mem[0]);
    pthread_cond_signal(&(list->condition));
  } while (list->proxyState != COMPLETE);
  pthread_mutex_unlock(&(list->mutex));

  len = strlen(msg);
  pthread_mutex_lock(&(list->mutex));
  for (i = 0; i < len; i++) {
    list->mem[0] = msg[i];
    list->serverState = WAITING_CONT_PRXY;

    if (i + 1 == len) {
      list->serverState = COMPLETE;
    }

    pthread_cond_signal(&(list->condition));
    pthread_cond_wait(&(list->condition), &(list->mutex));
  }
  pthread_mutex_unlock(&(list->mutex));
 
  if (destroyMemList(list, 1) < 0) {
    printf("Error destroying shared memory list!\n");
  }
  return 0;
}
