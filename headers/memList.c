#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include "memList.h"

/* This function builds an array of memory nodes, the number of which is
 * determined by the parameter passed in.  This array resides within
 * shared memory.  This will perform initialization on all the shared
 * semaphores as well.
 *
 * @param numNodes The length of the array of memnodes to be created.
 * @return An array of shared memory nodes.
 */
memnode* getMemList(int numNodes) {
  int exists;

  /* retrieve the list */
  memnode* retval = memOps_Create(SEGMENTNAME, sizeof(memnode) * numNodes, &exists);
  if (!retval) {
    #ifdef DEBUG
      printf("memList.c: Unable to allocate memnode list!\n");
    #endif
    return NULL;
  }

  /* initialize the synchronization variables, if needed */
  if (!exists) {
    int i;
    for (i = 0; i < numNodes; i++) {
      if (pthread_mutex_init(&(retval[i].mutex), NULL) != 0) {
        #ifdef DEBUG
          printf("memList.c: Unable to initialize mutex %d!\n", i);
        #endif
      }

      if (pthread_cond_init(&(retval[i].condition), NULL) != 0) {
        #ifdef DEBUG
          printf("memList.c: Unable to initialize condition var %d!\n", i);
        #endif
      }
      retval[i].spaceUsed = 0;
      retval[i].proxyState = IDLE;
      retval[i].serverState = IDLE;
    }
  }

  /* return the list */
  #ifdef DEBUG
    if (exists) {
      printf("memList.c: Shared memory list re-allocated from existing allocation.\n");
    } else {
      printf("memList.c: Shared memory list newly created.\n");
    }
  #endif
  return retval;
}

/* Given the list of shared memory nodes, this function will read each node
 * and return the first node it encounters with a state matching the state
 * that is passed in.  A prog parameter is used to differentiate between
 * the two process flags for each memory node.  If no node is encountered 
 * with a matching state, NULL is returned.
 *
 * NOTE: This function checks ONLY the state of the proxy, since the proxy
 *       is both the first and last entity to access the shared segment;
 *       it's activity is indicative of the activity of the node on a whole.
 *
 * @param list The list of shared memory nodes.
 * @param length The number of shared memory nodes.
 * @param s The state to be searched for.
 * @return A pointer to the first memory node with the appropriate state, or
 *         NULL if no such node is found.
 */
memnode* findState(memnode* list, int length, state_t s) {
  int i;

  /* loop */
  for (i = 0; i < length; i++) {
    if (list[i].proxyState == s) {
      return (list + i);
    }
  }

  /* nothing found */
  return NULL;
}

/* Given the list and the number of nodes in it, this function destroys
 * all references to the shared list and frees all the shared resources
 * associated with it, including the shared semaphores.
 *
 * @param list The array of shared memory nodes.
 * @param length The number of shared memory nodes.
 * @return 0 on success, -1 on failure.
 */
int destroyMemList(memnode* list, int length) {
  int i;

  /* first, destroy the synchronization variables */
  for (i = 0; i < length; i++) {
    int err;
    if ((err = pthread_mutex_destroy(&(list[i].mutex))) != 0) {
      #ifdef DEBUG
        printf("memList.c: Error destroying mutex %d!\n", i);
        if (err == EBUSY) {
          printf("memList.c: Mutex %d had error of EBUSY.\n", i);
        }
      #endif
    }
    if (pthread_cond_destroy(&(list[i].condition)) != 0) {
      #ifdef DEBUG
        printf("memList.c: Error destroying condition var %d!\n", i);
      #endif

      /* return -1; */
    }
  }

  /* now destroy the entire segment */
  if (memOps_Destroy(SEGMENTNAME, list, sizeof(memnode) * length) < 0) {
    #ifdef DEBUG
      printf("memList.c: Unable to destroy shared memory list!\n");
    #endif

    return -1;
  }

  /* success! */
  #ifdef DEBUG
    printf("memList.c: Deallocation of shared list successful.\n");
  #endif

  return 0;
}

/* For debugging purposes.  Prints out the fields of each memory node.
 * 
 * @param list The array of memory nodes.
 * @param length The length of the array.
 */
void printMemList(memnode* list, int length) {
  int i;

  for (i = 0; i < length; i++) {
    printf("--Node %d--\n", i + 1);
    printf("Space Used: %ld\n", list[i].spaceUsed);
    printf("Proxy: %d\n", list[i].proxyState);
    printf("Server: %d\n", list[i].serverState);
    printf("Memory: %s\n", list[i].mem);
    printf("\n");
  }
}

/* Given a new chunk of data and its length, this function will map
 * the new data into the shared memory region, overwriting anything
 * that was there previously.  The data will be zeroed out initially
 * to prevent any extraneous data remaining, then the new data written.
 *
 * WARNING: NO CHECKS are made to ensure that the amount of memory
 * written is within the allocated bounds!  This check is up to the
 * function's caller.
 *
 * @param node The node whose memory segment will be changed.
 * @param data The new data to be written to the segment.
 * @param length The length of the data to be written.
 * @return -1 on failure, otherwise it returns the number of bytes
 *         successfully written. Check this against length!
 */
long int setMapping(memnode* node, void* data, long int dataLength) {
  void* retval;

  /* first, zero everything out */
  memset(node->mem, 0, dataLength);

  /* second, map the new data */
  retval = memcpy(node->mem, data, dataLength);

  if (!retval) {
    #ifdef DEBUG
      printf("memList.c: Failure to copy new data to shared segment!\n");
    #endif

    return -1;
  }

  /* third, modify the data tracker to reflect how much data is present */
  node->spaceUsed = dataLength;

  /* finally, return the bytes written */
  return dataLength;
}

/* This function accesses the shared memory region either for an
 * existing metanode object, or creates the metanode and initializes
 * the data required for proper synchronized functioning between
 * server and proxy.
 *
 * @return A pointer to the metanode structure in shared memory.
 */
metanode* getMetanode(void) {
  int exists;
  metanode* retval = memOps_Create(METANODENAME, sizeof(metanode), &exists);
  if (!retval) {
    return NULL;
  }

  /* had it previously existed? */
  if (!exists) {

    /* set up the flags */
    retval->serverFlag = OFFLINE;
    retval->proxyFlag = OFFLINE;
    retval->numNodes = 0;
  }
  
  /* finished! */
  #ifdef DEBUG
    if (exists) {
      printf("memList.c: Metanode re-allocated from existing allocation.\n");
    } else {
      printf("memList.c: Metanode newly created.\n");
    }
  #endif
  return retval;
}

/* Destroys the metanode created by getMetanode.
 *
 * @param node The metanode to be destroyed.
 * @return 0 on success, -1 on failure.
 */
int destroyMetanode(metanode* node) {

  /* finally, destroy the rest of the metanode */
  if (memOps_Destroy(METANODENAME, node, sizeof(metanode)) < 0) {
    return -1;
  }

  /* success! */
  #ifdef DEBUG
    printf("memList.c: Metanode successfully destroyed.\n");
  #endif

  return 0;
}

/* This function performs all the necessary tasks to open up the
 * shared memory segment, make the necessary checks for existing
 * segments or creating new ones, mapping the memory, and closing
 * the file descriptor as well as returning the pointer to the
 * shared memory region.
 *
 * @param name The name of the shared memory region.
 * @param blockSize The size in bytes of the shared region.
 * @return A pointer to the shared block.
 */
void* memOps_Create(const char* name, long int blockSize, int* exists) {
  int fd;
  void* retval;

  (*exists) = 0;

  /* first, open the shared memory */
  if ((fd = shm_open(name, O_EXCL | O_RDWR | O_CREAT, 0777)) < 0) {
    if (errno == EEXIST) { /* segment already exists */
      (*exists) = 1;
      fd = shm_open(name, O_RDWR, 0777); /* this shouldn't fail */
      if (fd < 0) {
        #ifdef DEBUG
          printf("WTF MATE!\n");
        #endif

        return NULL;
      }
    } else { /* a real error */
      #ifdef DEBUG
        printf("memList.c: Unable to open shared segment!\n");
      #endif

      return NULL;
    }
  } else { /* brand-new creation, truncate to correct size */
    if (ftruncate(fd, blockSize) < 0) {
      #ifdef DEBUG
        printf("memList.c: Unable to truncate shared memory segment!\n");
      #endif

      return NULL;
    }
  }

  /* map the shared region */
  if ((retval = mmap(NULL, blockSize, PROT_READ | PROT_WRITE, 
                     MAP_SHARED, fd, 0)) == MAP_FAILED) {
    #ifdef DEBUG
      printf("memList.c: Unable to map shared memory segment!\n");
    #endif

    return NULL;
  }

  /* close the file descriptor */
  if (close(fd) < 0) {
    #ifdef DEBUG
      printf("memList.c: Unable to close file descriptor!\n");
    #endif

    return NULL;
  }

  /* everything checks */
  return retval;
}

/* This is the converse of _Create; it destroys the area in shared
 * memory associated with the name and the mapping, freeing those
 * resources.
 *
 * @param name The name of the shared memory segment.
 * @param mem The pointer to the mapped memory segment.
 * @param blockSize The size of the mapped region in bytes.
 * @return 0 on success, -1 on failure.
 */
int memOps_Destroy(const char* name, void* mem, long int blockSize) {
  /* first, unmap the region */
  if (munmap(mem, blockSize) < 0) {
    #ifdef DEBUG
      printf("memList.c: Unable to unmap shared memory segment!\n");
    #endif

    return -1;
  }

  /* second, unlink the shared memory region */
  if (shm_unlink(name) < 0) {
    #ifdef DEBUG
      printf("memList.c: Unable to unlink shared memory segment!\n");
    #endif

    return -1;
  }

  /* we're done here */
  return 0;
}

/* for testing */
/*
int main(int argc, char** argv) {
  memnode* list;
  metanode* n = getMetanode();
  if (destroyMetanode(n) < 0) {
    printf("Error destroying metanode.\n");
    exit(1);
  }

  list = getMemList(10);
  printMemList(list, 10);
  if (destroyMemList(list, 10) < 0) {
    printf("Error destroying memory list.\n");
    exit(1);
  }

  if (pthread_mutex_lock(&(list[0].mutex)) != 0) {
    printf("Error locking mutex!\n");
  } else {
    printf("Mutex successfully locked.\n");
  }

  return 0;
}
*/
