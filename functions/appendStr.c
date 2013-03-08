#ifndef _APPENDSTR_
#define _APPENDSTR_

#include "../headers/constants.h"

/* This function appends the string toAdd to the first parameter, 
 * content, and returns that pointer.  This is essentially a 
 * glorified strcat().
 *
 * @param content The original string to be reallocated and lengthened.
 * @param toAdd The string to be appended.
 * @return The successfully appended string, or NULL on error.
 */
char* appendStr(char* content, char* toAdd) {
  char* link;
  char* toAssign;
  int i;

  if (!content) { /* small sanity check */
    i = strlen(toAdd);
    content = calloc((i + 1), sizeof(char));
    if (!content) { /* failure */
      return NULL;
    }

    /* just copy the memory pointed at by toAdd to content */
    content = memcpy(content, toAdd, i);
    return content;
  }

  /* reaching this point indicates there is something attached to content */
  i = (strlen(toAdd) * 2) + strlen("<a href=\"\"></a><br />\n") + 1;
  link = calloc(i, sizeof(char));
  if (!link) { /* failure */
    return NULL;
  }
  snprintf(link, (i - 1), "<a href=\"%s\">%s</a><br />\n", toAdd, toAdd);

  /* don't want to make links out of the footer of the page */
  if (strstr(toAdd, SERVER_NAME)) {
    toAssign = toAdd;
  } else { /* reallocate space with enough room for strcat */
    toAssign = link;
  }

  content = realloc(content, sizeof(char) * (strlen(content) + strlen(toAssign) + 1));
  if (!content) {
    return NULL;
  }

  content = strncat(content, toAssign, strlen(content) + strlen(toAssign));
  free(link);
  return content;
}


#endif /* _APPENDSTR_ */
