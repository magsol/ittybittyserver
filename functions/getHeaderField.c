#ifndef _GETHEADERFIELD_
#define _GETHEADERFIELD_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../headers/constants.h" /* for EOL */
#include "strcasestr.c" /* for case-ignoring matching */

/* Given the header for an HTTP request or response, this 
 * function will extract the value for a specific header and
 * return a dynamically allocated string containing only
 * the value.
 *
 * In order to ensure proper string behavior, this function
 * dynamically allocates a string to hold a duplicate of the
 * value as found in the header.  Thus, the caller will need
 * to explicitly free the allocated return value to avoid
 * memory leaks.
 *
 * @param header The header of the HTTP request/response.
 * @param headerLength The length in bytes of the header.
 * @param field The field whose value has been requested.
 * @return The value of the field as specified by "field," or NULL
 *         if the field was not found or an error occurred.
 */
char* getHeaderField(void* header, long int headerLength, char* field) {
  char* toSearch = calloc(headerLength + 1, sizeof(char));
  char* toFind, *retVal = NULL;
  if (!toSearch) {
    return NULL;
  }

  /* copy over the header's contents so we have a viable string to search */
  strncpy(toSearch, (char*)header, headerLength);
  /* now, search the string for the field */
  if ((toFind = strcasestr(toSearch, field))) {
    char* endOfLine = strstr(toFind, EOL);
    if (!endOfLine) { /* should never, ever happen */
      free(toSearch);
      return NULL;
    }
    
    /* we have the start of the field, and the end of the value */
    #ifdef DEBUG
     /*  printf("%s\n", toFind); */
    #endif
    
    /* find the start of the value */
    toFind = strchr(toFind, ':');
    if (!toFind) { /* again, shouldn't ever happen */
      free(toSearch);
      return NULL;
    }

    /* skip over the colon */
    toFind++;
    
    /* skip all white space */
    while (toFind[0] == ' ') {
      toFind++;
    }

    /* allocate the return string */
    retVal = calloc((endOfLine - toFind + 1), sizeof(char));
    if (!retVal) {
      free(toSearch);
      return NULL;
    }

    /* copy the value */
    strncpy(retVal, toFind, (endOfLine - toFind));
  }

  /* free the temporary search string and return the value */
  free(toSearch);
  return retVal;
}

/* A small extension of the above function, this processes the 
 * "Host" field and returns the port number from the request, or
 * -1 if an error occurs or the parameter was invalid.
 *
 * @param server The full value for the host field.  Could be
 *               the return value from getHeaderField() with "Host".
 * @return An integer corresponding to the correct port number. 80
 *         is the default, or -1 if an error occurs.
 */ 
int getPortNumber(char* server) {
  int portNum = 80;
  char* colon;

  /* first, sanity check */
  if (!server) {
    return -1;
  }

  /* second, find out if there's a colon at the END of the URI */
  if ((colon = strrchr(server, ':'))) {
    /* test to make sure this isn't from http: */
    if (*(colon - 1) != 'p') { /* we have a winnar! */
      char num[100];
      colon++; /* increment to start of port number */
      memset(&num, 0, sizeof(num));
      strncpy(num, colon, strlen(colon));
      portNum = atoi(num);
    }
  }

  /* return the port number */
  return portNum;
}

/* Following the call to getPortNum(), this function will strip
 * the trailing the port number off the server and return a string
 * with only the URI.
 *
 * @param server The full name of the server, a la a call to getHeaderField()
 *               with "Host".
 * @return The full name of the server, without the port number. This value
 *         must be explicitly freed by the caller.
 */
char* chopPortNum(char* server) {
  char* retVal = NULL, *eos;
  char ch;

  if (!server) {
    return NULL;
  }

  /* attach eos to the end of server */
  eos = server + (strlen(server) - 1);

  /* loop backwards until we find the URI itself */
  ch = eos[0];
  while ((ch >= '0' && ch <= '9') || ch == ' ' || ch == ':') {
    eos--;
    if (ch == ':') { /* it could be 127.0.0.1 */
      break;
    }
    ch = eos[0];
  }

  /* eos now points to the last character of the valid URI */
  retVal = calloc((eos - server) + 2, sizeof(char));
  if (!retVal) {
    return NULL;
  }

  /* copy everything over */
  strncpy(retVal, server, (eos - server) + 1);

  /* finish up! */
  return retVal;
}

#endif /* _GETHEADERFIELD_ */
