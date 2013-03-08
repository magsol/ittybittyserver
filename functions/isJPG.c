#ifndef _ISJPG_
#define _ISJPG_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "getHeaderField.c"
#include "strcasestr.c"

/*
 * This determines whether there is a request for a JPG image 
 * in the header that is provided.
 *
 * @param toSearch The header
 * @param length The length of the header, in bytes.
 * @return 1 if there is a request for a JPG, 0 otherwise.
 */
int isJPG(void* toSearch, long int length) {
  char* field;
  char buffer[100];

  /* first, the easy check */
  field = getHeaderField(toSearch, length, "\nContent-Type");
  if (strcasestr(field, ".jpg") || strcasestr(field, "jpeg")) { /* easy */
    free(field);

    #ifdef DEBUG
      printf("isJPG.c: Request for JPG found in Content-Type.\n");
    #endif

    return 1;
  }

  free(field);

  /* arriving here means...not so easy */
  memset(&buffer, 0, sizeof(buffer));
  memcpy(buffer, toSearch, 10);
  if (!strstr(buffer, "GET")) { /* uhhh... */
    #ifdef DEBUG
      printf("isJPG.c: Unable to find GET request in header.  Malformed header?\n");
    #endif

    return 0;
  }
  memcpy(buffer, toSearch, 99);
  if (!strcasestr(buffer, ".jpg")) { /* yeah, nothing here */

    #ifdef DEBUG
      printf("isJPG.c: No request for JPG found in header.\n");
    #endif

    return 0;
  } else { /* got one the old fashioned way! */

    #ifdef DEBUG
      printf("isJPG.c: Request for JPG found in start of header.\n");
    #endif

    return 1;
  }
}

#endif /* _ISJPG_ */
