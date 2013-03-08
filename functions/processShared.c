#ifndef _PROCESSSHARED_
#define _PROCESSSHARED_

#include <pthread.h>
#include <stdio.h>
#include "../headers/constants.h"
#include "../headers/memList.h"

/* 
 * Implement mutually exclusive communication functionality to
 * send a message over shared memory which may not initially fit
 * in its entirety in the shared block.
 *  
 * SERVER SIDE:
 * 
 * STEP 1: Lock mutex.
 * STEP 2: Set serverState to WAITING_CONT_PRXY to signal to the proxy
 *         that server is ready and waiting to process a request.
 * STEP 3: Signal the condition variable.
 * STEP 4: Wait on proxy's signal.
 * STEP 5: Read shared memory.
 * STEP 6: If proxy signal is COMPLETE, set serverState to BUSY and
 *         continue to STEP 7.
 *         If proxy signal is WAITING_CONT_SRVR, buffer the partial
 *         request received, and go to STEP 2.
 * STEP 7: Generate response.
 * STEP 8: Write maximum possible data to the shared memory.
 * STEP 9: If entire response has been written, set serverState to
 *         COMPLETE, signal condition variable, and continue to STEP 10.
 *         If there is more to be written, set serverState to 
 *         WAITING_CONT_PRXY, signal condition variable, wait for 
 *         corresponding signal from proxy, and go to STEP 7.
 * STEP 10: Unlock mutex. Reset serverState to IDLE and exit.
 *
 * PROXY SIDE:
 *
 * STEP 1: Lock mutex.
 * STEP 2: Set proxyState to WAITING_INIT_SRVR.
 * STEP 3: Wait on server's signal.
 * STEP 4: Generate request.
 * STEP 5: Set proxyState to BUSY and write maximum possible data to 
 *         the shared memory.
 * STEP 6: If entire request has been written, set proxyState to COMPLETE,
 *         signal condition variable, and continue to STEP 7.
 *         If there is more to be written, set proxyState to 
 *         WAITING_CONT_SRVR, signal condition variable, wait for
 *         corresponding signal from server, and go to STEP 4.
 * STEP 7: Wait on server's signal.
 * STEP 8: Read shared memory.
 * STEP 9: If server signal is COMPLETE, set proxyState to BUSY and 
 *         continue to STEP 10.
 *         If server signal is WAITING_CONT_PRXY, buffer partial response,
 *         set proxyFlag to WAITING_CONT_SRVR, signal condition variable,
 *         and go to STEP 7.
 * STEP 10: Unlock mutex.  Reset proxyState to IDLE and exit.
 */


/* This function takes care of the synchronization involved in shared
 * memory communication for both the proxy and the server by managing 
 * mutexes and condition variable signaling involved in sending a message
 * in its entirety to the other entity.
 *
 * If the message send is incomplete, it will wait for a response from the
 * other entity, which signals that it is ready for more information to
 * be sent.
 *
 * NOTE: Lock mutex before calling this function!
 *
 * @param header The header of the message to be sent.
 * @param headerLen The length, in bytes, of the header.
 * @param body The body of the message to be sent.
 * @param bodyLen The length, in bytes, of the body.
 * @param sharedNode The memnode through which this message will be sent.
 */
void sendShared(char* header, long int headerLen, void* body, 
                long int bodyLen, memnode* sharedNode) {
  long int totalBytes = 0;
  void* total;
  int isServer;
  /* this determines whether the server or proxy called this function */
  state_t* theState = (sharedNode->proxyState == COMPLETE ? 
                       &(sharedNode->serverState) : 
                       &(sharedNode->proxyState));
  isServer = (sharedNode->proxyState == COMPLETE ? 1 : 0);

  /* map everything */
  total = malloc(sizeof(char) * (headerLen + bodyLen));
  if (!total) { /* asdkjfsald */
    printf("Unable to allocate header/body buffer!\n");
    return;
  }
  memcpy(total, header, headerLen);
  memcpy(((char*)total + headerLen), body, bodyLen);

  /* loop on the data */
  while (totalBytes < (headerLen + bodyLen)) {
    long int toWrite;

    (*theState) = BUSY;
    if (((headerLen + bodyLen) - totalBytes) > SEGMENTSIZE) {
      /* we'll need multiple passes to write all the data */
      toWrite = SEGMENTSIZE;
    } else {
      /* small enough of a data packet to write in a single segment */
      toWrite = (headerLen + bodyLen) - totalBytes;
    }

    /* send it to shared memory */
    if ((totalBytes += setMapping(sharedNode, ((char*)total + totalBytes),
                                  toWrite)) < 0) {
      printf("Fatal application error: Unable to map to shared memory!\n");
      exit(MEMALLOC_FAILURE);
    }

    #ifdef DEBUG
      printf("sendShared.c: %ld bytes out of %ld sent.\n", totalBytes, (headerLen + bodyLen));
    #endif

    /* do we need to make another pass? */
    if (totalBytes == (headerLen + bodyLen)) { /* nope */
      #ifdef DEBUG
        printf("sendShared.c: Completed send!.\n");
      #endif

      (*theState) = COMPLETE;
      pthread_cond_signal(&(sharedNode->condition));
      if (isServer) {
        pthread_cond_wait(&(sharedNode->condition), &(sharedNode->mutex));
      }
    } else { /* more to go */
      (*theState) = (isServer ? WAITING_CONT_PRXY : WAITING_CONT_SRVR);
      pthread_cond_signal(&(sharedNode->condition));
      pthread_cond_wait(&(sharedNode->condition), &(sharedNode->mutex));
    }
  }

  free(total);
  /* all done! */
}

/* This is the conjugate of sendShared().  This will assume the caller
 * currently has the mutex locked and is attempting to receive a chunk
 * of data from the shared memory segment.  This function will read the
 * number of bytes from the segment as specified by the spaceUsed variable
 * within the shared segment, and will return a dynamically allocated copy
 * to the caller.
 *
 * NOTE: Due to possible size issues, this function will only execute
 * a receive ONCE.  The caller is responsible for placing this function
 * within a loop to obtain data which exceeds the size of the shared
 * memory segment.  Effectively, this function can be called repeatedly
 * until it returns NULL.
 *
 * @param sharedNode The shared segment from which data will be read.
 * @return A dynamically allocted buffer containing the shared memory
 *         data, or NULL if no data is found.
 */
void* receiveShared(memnode* sharedNode) {
  void* retval;

  /* first, check if there is any data at all */
  if (sharedNode->spaceUsed == 0) {
    #ifdef DEBUG
      printf("processShared.c: No data to be read from shared space.\n");
    #endif
    return NULL;
  }

  /* allocate the return space */
  retval = malloc(sizeof(char) * sharedNode->spaceUsed);
  if (!retval) {
    #ifdef DEBUG
      printf("processShared.c: Unable to allocate return buffer!\n");
    #endif
    return NULL;
  }
  /* copy the message */
  memcpy(retval, sharedNode->mem, sharedNode->spaceUsed);

  #ifdef DEBUG
    printf("processShared.c: %ld bytes read from shared memory.\n", sharedNode->spaceUsed);
  #endif

  /* return the result */
  return retval;
}

#endif /* _PROCESSSHARED_ */
