#ifndef _STRCASESTR_
#define _STRCASESTR_

#include <string.h>
#include <ctype.h>

/* Since there is no standard implementation of a case-insensitive
 * string comparison, I have opted to implement my own.  This function
 * looks for an occurrence of needle within haystack, ignoring the 
 * case of both arguments.
 *
 * @param haystack The string to be searched.
 * @param needle The string to search for.
 * @return A pointer to the first occurrence of needle in haystack,
 *         NULL if nothing is found.
 */
char* strcasestr(const char* haystack, const char* needle) {
  int needleLength, haystackLength;
  int i, j;
  char* retVal = NULL;

  /* sanity check */
  if (!haystack || !needle) { /* doh */
    return NULL;
  }

  needleLength = strlen(needle);
  haystackLength = strlen(haystack);
  for (i = 0; i < haystackLength - needleLength; i++) {
    int isMatch = 1;
    for (j = 0; j < needleLength; j++) {
      char ch1 = toupper(haystack[i + j]);
      char ch2 = toupper(needle[j]);
      if (ch1 != ch2) { /* not equal, bummer */
        isMatch = 0;
        j = needleLength; /* break the loop */
      }
    }
    if (isMatch) { /* WE HAVE A MATCH */
      retVal = (char*)haystack + i;
      return retVal;
    }
  }

  /* it's null? */
  return retVal;
}

#endif /* _STRCASESTR_ */
