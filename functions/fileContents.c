#ifndef _FILECONTENTS_
#define _FILECONTENTS_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

/* Given a file name and the size of the content within that file,
 * this function maps the file's contents to an area in memory
 * referenced by the return value.
 *
 * Here's the caveat regarding this function: it will be up the
 * caller to retrieve the file's contents from memory using the 
 * returned pointer and the size that was passed in.  It is
 * also the caller's responsibility to unmap the mapped memory, 
 * as well as close the open file descriptor that was passed
 * to this function.
 *
 * @param filedesc The file descriptor of the open file resource.
 * @param size The size of the file's contents.
 * @return A pointer to memory where the content resides, or NULL on
 *         failure. 
 */
void* fileContents(int filedesc, off_t size) {
  void* retVal;

  /* a little error checking */
  if (filedesc < 0) {
    return NULL;
  }

  /* map it! */
  /* PROT_READ: Indicates this memory can be read again */
  /* MAP_SHARED: Indicates remappings are possible */
  retVal = mmap(0, size, PROT_READ, MAP_SHARED, filedesc, 0);
  if (retVal == (void*)-1) { /* failure */
    close(filedesc);
    return NULL;
  }

  /* return the address to the mapped memory */
  return retVal;
}

#endif /* _FILECONTENTS_ */
