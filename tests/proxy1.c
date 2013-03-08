#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include "../headers/memList.h"

/* PROXY */

int main(int argc, char** argv) {
  int i, len;
  memnode* list = getMemList(1);
  char* msg = "I heart huckabees";

  len = strlen(msg);

  /* wait for the client to appear */
  printf("Waiting...\n");
  while (list->proxyState != BUSY);
  pthread_mutex_lock(&(list->mutex));
  list->proxyState = WAITING_INIT_SRVR; /* this signals the server */

  if (list->serverState != WAITING_CONT_PRXY) {
    pthread_cond_wait(&(list->condition), &(list->mutex));
  }

  for (i = 0; i < len; i++) {
    list->mem[0] = msg[i];
    list->proxyState = WAITING_CONT_SRVR;

    if (i + 1 == len) {
      list->proxyState = COMPLETE;
      pthread_cond_signal(&(list->condition));
    } else {
      pthread_cond_signal(&(list->condition));
      pthread_cond_wait(&(list->condition), &(list->mutex));
    }
  }
  pthread_mutex_unlock(&(list->mutex));

  pthread_mutex_lock(&(list->mutex));
  do {
    pthread_cond_wait(&(list->condition), &(list->mutex));
    printf("%c ", list->mem[0]);
    pthread_cond_signal(&(list->condition));
  } while (list->serverState != COMPLETE);
  pthread_mutex_unlock(&(list->mutex));
  
  return 0;
}
