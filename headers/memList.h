#ifndef _MEMLIST_
#define _MEMLIST_

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <pthread.h>

#include "constants.h" /* for naming schemes and SEGMENTSIZE */

/* This stores the information pertinent to building shared memory block
 * lists to be shared between the server and proxy processes.
 *
 * When one or both of the services are run with the "-o" optimization
 * flag, they allocate a single block of shared memory, "metanode", with
 * a single metanode in it.  They set their own respective flag to 
 * indicate their readiness to utilize shared memory instead of sockets
 * in order to communicate.
 *
 * The server will the process which initially creates the shared memory;
 * thus, the number of shared memory segments will be equivalent (or very
 * close) in number to the quantity of worker threads (1-to-1 ratio of
 * worker threads to shared memory segment).
 *
 * States within each shared memory segment will act as surrogate 
 * semaphores, synchronizing access to the data held within.  There are
 * two state indicators per segment: proxy and server.
 *
 * IDLE: No activity.
 * BUSY: Currently reading or writing - do not touch!
 * WAITING: No activity; waiting on a response before proceeding.
 * COMPLETE: Ready to be reset to IDLE.
 *
 * There are two possible transaction patterns:
 *
 * 1)
 * a. PROXY posts a request for the SERVER.
 * b. SERVER reads the request.
 * c. SERVER posts full response.
 * d. PROXY reads response.
 *
 * 2)
 * a. PROXY posts a partial request.
 * b. SERVER reads partial request and waits (repeat a-b until PROXY posts COMPLETE).
 * c. SERVER processes request, builds and posts partial response.
 * d. PROXY reads partial response and waits (repeat c-d until SERVER posts COMPLETE).
 * e. PROXY posts COMPLETE once finished reading response.  Both states on
 * COMPLETE are reset to IDLE.
 *
 * This will also serve as a wrapper for the shared memory functionality, by
 * way of two functions:
 *
 * memOps_Create()
 * memOps_Destroy()
 *
 * Both will take care of all the space allocation, truncating, error 
 * checking, and mapping, as well as unlinking and deleting.
 */

/* the flag types */
typedef enum flag {
  OFFLINE,
  ONLINE
} flag;

typedef enum state_t {
  IDLE,               /* ready for use */
  BUSY,               /* DO NOT TOUCH - operation in progress! */
  WAITING_INIT_SRVR,  /* proxy is waiting for initial server contact */
  WAITING_CONT_PRXY,  /* server is waiting for proxy to continue read/write */
  WAITING_CONT_SRVR,  /* proxy is waiting for server to continue read/write */
  COMPLETE            /* waiting to be reset to IDLE */
} state_t;

/* a singleton */
typedef struct metanode {
  flag serverFlag;
  flag proxyFlag;
  int numNodes;
} metanode;

/* a memory node */
typedef struct memnode {
  char mem[SEGMENTSIZE];
  long int spaceUsed;
  state_t proxyState;
  state_t serverState;
  pthread_mutex_t mutex;
  pthread_cond_t condition;
} memnode;

/* memory list functions */
memnode* getMemList(int numNodes);
memnode* findState(memnode* list, int length, state_t s);
int destroyMemList(memnode* list, int length);
void printMemList(memnode* list, int length);

/* metanode functions */
metanode* getMetanode(void);
int destroyMetanode(metanode* node);

/* memory functions */
long int setMapping(memnode* node, void* data, long int dataLength);
void* memOps_Create(const char* name, long int blockSize, int* exists);
int memOps_Destroy(const char* name, void* mem, long int blockSize);

#include "memList.c"
#endif /* _MEMLIST_ */
