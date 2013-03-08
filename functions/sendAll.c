#ifndef _SENDALL_
#define _SENDALL_

#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>

/* Given a socket identifier, the address of an integer, and 
 * a pointer to the block of data to be sent, this function
 * sends all data possible over the wire.  The only case in 
 * which not all the data will be sent is if an error occurs.
 *
 * @param socket The socket identifier.
 * @param buf A pointer to the buffer of data to be sent.
 * @param len A pointer to an integer; upon the function's return,
 *            the value pointed at by len will contain the number of
 *            bytes sent.
 * @return An error code on failure, 0 on success.
 */
int sendAll(int socket, void* buf, long int* len) {
  long int total = 0; /* everything sent */
  int bytesleft = *len;
  int i;

  /* loop until everything has been sent or an error occurs */
  while (total < (*len)) {
    i = send(socket, ((char*)buf + total), bytesleft, MSG_NOSIGNAL);
    if (i < 0) { /* an error occurred */
      return i;
    }
    total += i;
    bytesleft -= i;
  }

  /* everything was sent!  store that number */
  *len = total;

  return 0;
}

#endif /* _SENDALL_ */
