#ifndef _COPYLENGTH_
#define _COPYLENGTH_

#include <stdlib.h>
#include <string.h>

#include "../headers/constants.h"

/*
 * Given the string parameter, pointing to the HTTP request
 * at point of "Content-Length", this function returns the 
 * integer value of the content length.
 *
 * @param Character pointer to the letter "C".
 * @return Integer value after Content-Length header.
 */
int copyLength(char* str) {
  char* increment;
  int length, retval;
  char* end = strstr(str, EOL);

  if (!end) { /* this should never ever happen... */
    return -1;
  }

  str += strlen("Content-Length: "); /* point to first number */
  length = end - str + 1; /* pointer arithmetic! */
  increment = calloc(length, sizeof(char));
  if (!increment) { /* failure to allocate memory */
    return -1;
  }

  /* copy over letters */
  strncpy(increment, str, length);

  /* obtain integer value */
  retval = atoi(increment);

  /* clean up */
  free(increment);

  /* get out */
  return retval;
}


#endif /* _COPYLENGTH_ */
