#ifndef _GETSTATUSCODE_
#define _GETSTATUSCODE_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Given the header and the length of the header, this function
 * will return the status code found within it, or 0 if no status
 * was found.  -1 is returned on error.
 *
 * @param header The header to be searched.
 * @param headerLength The length, in bytes, of the header.
 * @return An integer HTTP status code.
 */
int getStatusCode(void* header, long int headerLength) {
  int code, size, i;
  char* buffer; /* temporary buffer for the status code */
  char* ptr;
  char num[50];

  /* we assume that the status code is near the beginning of the header;
   * thus, if nothing is found within the first, say, 100 bytes, it's
   * safe to say the status code isn't there at all
   */

  memset(&num, 0, sizeof(num));
  size = (headerLength > 100 ? 101 : headerLength + 1);
  buffer = calloc(size, sizeof(char));
  if (!buffer) { /* memory allocation error */
    return -1;
  }
  strncpy(buffer, (char*)header, size - 1);
  ptr = strchr(buffer, ' ');
  ptr++;

  /* increment along */
  for (i = 0; ptr[0] != ' ' && i < size; i++) {
    buffer[i] = ptr[0];
    ptr++;
  }

  /* turn that string into something usable */
  code = atoi(buffer);
  free(buffer);
  return code;
}


#endif /* _GETSTATUSCODE_ */
