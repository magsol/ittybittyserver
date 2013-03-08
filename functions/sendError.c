#ifndef _SENDERROR_
#define _SENDERROR_

#include <string.h>
#include "../headers/conList.h"
#include "../headers/memList.h"
#include "../headers/constants.h"
#include "sendResponse.c"

/* A wrapper for sending an HTTP error message from the server
 * to the client.
 *
 * @param status This is the status integer.  Common statuses include
 *               404 Not Found, 503 Server Error, etc
 * @param title The title of the error page, corresponding to the status
 * @param headers Any additional headers.
 * @param text The body text of the error page.
 * @param c The connection through which to send the error message.
 * @param shared Integer indicating whether this connection is shared memory.
 */
void sendError(int status, char* title, char* headers,
               char* text, void* c, int shared) {

  char buf[10000];
  int strLen;

  snprintf(buf, (sizeof(buf) - 1), "<html><head><title>%d %s</title></head>\n<body bgcolor=\"#CC9999\"><h4>%d %s</h4>\n%s<hr><address><a href=\"%s\">%s</a></address>\n</body></html>\n", status, title, status, title, text, SERVER_URL, SERVER_NAME);

  strLen = strlen(buf);
  sendResponse(status, title, headers, "text/html", strLen, buf, c, shared);
}

#endif /* _SENDERROR_ */
