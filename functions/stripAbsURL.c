#ifndef _STRIPABSURL_
#define _STRIPABSURL_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "getHeaderField.c"
#include "../headers/constants.h" /* for EOL */

/* This function operates for the purposes of the proxy server.  When a 
 * request arrives from a client configured to use the proxy, the path
 * after GET will be absolute.  The proxy will strip out this absolute
 * request and replace it with a relative request.  This function strips
 * out the http://[Host] and leaves only the relative path.
 *
 * @param header The initial header, to be modified.
 * @param oldLength The number of bytes in the header parameter.
 * @return The length in bytes of the new header.
 */
void* stripAbsURL(void* header, long int oldLength, long int* newL) {
  void* newHeader;
  long int newLength, firstHalf, secondHalf;
  char* http = "http://";
  void* start, *end;
  char toFind[10000];
  char* host = getHeaderField(header, oldLength, "\nHost");
  if (!host) { /* this is bad */

    #ifdef DEBUG
      printf("No host found in header!\n");
    #endif

    return NULL;
  }

  /* set new header length (+1 for slash) */
  newLength = oldLength - strlen(http) - strlen(host);
  (*newL) = newLength;

  #ifdef DEBUG
    printf("Header length changed from %ld to %ld.\n", oldLength, newLength);
    printf("Host counted: |%s%s|\n", http, host);
    printf("Host length: %d\n", strlen(http) + strlen(host));
  #endif

  newHeader = calloc(newLength, sizeof(char));
  if (!newHeader) { /* crap on a stick */

    #ifdef DEBUG
      printf("Error allocating memory for resized header.\n");
    #endif

    return NULL;
  }

  /* create a duplicate of the part of the header we want to replace */
  memset(&toFind, 0, sizeof(toFind));
  snprintf(toFind, (sizeof(toFind) - 1), "%s%s", http, host);
  free(host); /* no memory leaks, kthx */

  /* position the two pointers at the start and end of the abs URL */
  start = strchr((char*)header, ' '); /* first space */
  end = ((char*)start) + strlen(toFind) + 1; /* banking on trailing slash */

  /* do insanely complicated memory mappings... */
  firstHalf = (char*)start - (char*)header + 1;
  secondHalf = (oldLength - (strlen(toFind))) - firstHalf;
  memcpy((char*)newHeader, (char*)header, firstHalf);
  memcpy(((char*)newHeader + ((char*)start - (char*)header + 1)), end, secondHalf); 
 
  /* first step copies the protocol and the blank space */
  /* the second step copies from the first / to the end of the header */

  #ifdef DEBUG
    printf("Test length: %d\n", strlen(toFind));
    printf("Old Header: %ld\nNew Header: %ld\n", oldLength, newLength);
    printf("Pre-bytes copied: %ld\n", firstHalf);
    printf("Post-bytes copied: %ld\n", secondHalf);
  #endif

  /* return the new header */

  return newHeader;
}

#endif /* _STRIPABSURL_ */
