#ifndef _STRDECODE_
#define _STRDECODE_

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* makes things...cleaner */
int hexit(char c);

/* This function strips out all the blah hexadecimal HTML encodings
 * from a URL.
 *
 * TYPICALLY, this function will be called with the SAME pointer
 * passed for BOTH arguments.  The effective result is that the
 * string is transformed.  Since the transformation will always
 * contain at most the same number of characters as the original, 
 * we don't have to worry about overflowing the character buffer.
 *
 * @param to Will contain the transformed string, minus the encodings.
 * @param from The origial string, left untouched.
 */
void strDecode(char* to, char* from) {
  for ( ; *from != '\0'; ++to, ++from ) {
    if ( from[0] == '%' && isxdigit( from[1] ) && isxdigit( from[2] ) ) {
      *to = hexit( from[1] ) * 16 + hexit( from[2] );
      from += 2;
    } else {
      *to = *from;
    }
  }

  *to = '\0';
}

/* Converts a single character into its hexadecimal representation */
int hexit(char c) {

  if ( c >= '0' && c <= '9' ) {
    return c - '0';
  }
  if ( c >= 'a' && c <= 'f' ) {
    return c - 'a' + 10;
  }
  if ( c >= 'A' && c <= 'F' ) {
    return c - 'A' + 10;
  }

  return 0;           /* shouldn't happen, we're guarded by isxdigit() */
}

#endif /* _STRDECODE_ */
