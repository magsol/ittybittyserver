#ifndef _SENDRESPONSE_
#define _SENDRESPONSE_

#include <string.h>
#include "../headers/constants.h"
#include "../headers/conList.h"
#include "../headers/memList.h"
#include "sendAll.c"
#include "processShared.c"

/* This function sends an HTTP response over the wire.
 *
 * @param status The return code of the page.  200 indicates OK, otherwise
 *               indicates an error occurred.
 * @param title The title of the HTML page.
 * @param headers Any additional headers.
 * @param mime The MIME encoding type of the page.
 * @param length The length in bytes of the body.
 * @param body The body of the HTTP transfer.
 * @param c The node containing the socket identifier, OR the shared memory.
 * @param shared Integer indicating whether this connection is shared memory.
 */
void sendResponse(int status, char* title, char* headers,
                  char* mime, off_t length, void* body, void* c, 
                  int shared) {

  char header[10000];
  off_t headerLen;

  snprintf(header, (sizeof(header) - 1), "%s %d %s %s%sContent-Length: %ld %sContent-Type: %s %s%s", PROTOCOL, status, title, EOL, (headers ? headers : ""), length, EOL, mime, EOL, EOL);

  headerLen = strlen(header);

  /* shared or socket connection? */
  if (shared) {
    memnode* sharedNode = (memnode*)c;

    /* intermediate steps, depending on caller */
    sendShared(header, headerLen, body, length, sharedNode);

    #ifdef DEBUG
      printf("sendResponse.c: sendShared() successful, unlocking mutex.\n");
    #endif

    /* STEP 10 */
    pthread_mutex_unlock(&(sharedNode->mutex));

    #ifdef DEBUG
      printf("sendResponse.c: Mutex unlocked.\n");
    #endif

  } else {
    connection* connNode = (connection*)c;
    /* send the entire header package */
    if (sendAll(connNode->conn, header, &headerLen) < 0) {
      printf("Error sending header!\n");
    }

    #ifdef DEBUG
      printf("sendResponse.c: Header length is %ld\n", headerLen);
    #endif

    /* now send the body */
    if (sendAll(connNode->conn, body, &length) < 0) {
      printf("Error sending body!\n");
    }

    #ifdef DEBUG
      printf("sendResponse.c: Body length is %ld\n", length);
    #endif
  }
}

  /* send the entire header package */
  /*
  sem_wait(shMeta->semaphore);
  n->state = BUSY;
  bytesWritten = setMapping(n, header, headerLen);
  n->state = WAITING_FOR_PROXY;
  sem_post(shMeta->semaphore);
  
  #ifdef DEBUG
    printf("Body Length: %ld\n", bytesWritten);
  #endif
  */
  /* now send the body */
  /*
  bytesWritten = 0;
  while (bytesWritten < length) {
    while (n->state != WAITING_FOR_SERVER) {
      sem_wait(shMeta->semaphore);
    }

    n->state = BUSY;
    bytes = setMapping(n, (char*)body + bytesWritten, length - bytesWritten);
    bytesWritten += bytes;
    n->state = WAITING_FOR_PROXY;
    sem_post(shMeta->semaphore);
  }
  */
  /* finished! */
  /*
  n->state = COMPLETE;

  #ifdef DEBUG
    printf("Body Length: %ld\n", bytesWritten);
  #endif
}
*/
#endif /* _SENDRESPONSE_ */
