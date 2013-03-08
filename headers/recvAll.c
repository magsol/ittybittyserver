#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ctype.h>

#include "../functions/getHeaderField.c"
#include "../functions/sendAll.c"
#include "../functions/getStatusCode.c"

/* Given a socket identifier and the address of an integer, 
 * this function will read everything possible from the socket
 * and return it, putting the size of the received block
 * into the length parameter.
 *
 * Checks are made to determine if the server or the client is
 * accessing this function.  If there is a "Content-Length" string
 * found in the received data, it is assumed that the server
 * sent the data and the client is receiving it, and thus a body
 * will be searched for of length Content-Length.  If no such field
 * is found, data is received until the string "\r\n\r\n" is found,
 * delineating the end of the headers and thus the end of the 
 * transmission, implying it was sent by the client to the server.
 *
 * @param socket The socket identifier.
 * @param bodyLength The size of the body. Its value is inconsequential
 *                   until after the function call, at which point its value
 *                   contains the size of the returned block's body, or 0
 *                   if it was only a header.
 * @param headerLength The size of the header.  Its value is inconsequential
 *                     until after the function call, at which point its value
 *                     contains the size of the returned header's body in bytes.
 * @return A pointer to the received block of data, header AND body.
 */
void* recvAll(int socket, long int* headerLength, long int* bodyLength) {
  long int headerLen, bodyLen;
  long int bytesReceived = 0;
  int bytes = 0;

  void* header = recvHeader(socket, &headerLen, &bodyLen);
  if (!header) {
    return NULL;
  }

  (*headerLength) = headerLen;
  if (bodyLen == 0) { /* just a client request */
    (*bodyLength) = 0;
    return header;
  }

  /* it's a server response */
  header = realloc(header, sizeof(char) * (headerLen + bodyLen));
  if (!header) {

    #ifdef DEBUG
      printf("Unable to allocate memory for expanded header + body!\n");
    #endif
    
    return NULL;
  }

  while (bytesReceived < bodyLen) {
    char inputBuffer[20000];
    memset(&inputBuffer, 0, sizeof(inputBuffer));
    bytes = recv(socket, inputBuffer, (sizeof(inputBuffer) - 1), 0);
    if (bytes < 0) { /* error */

      #ifdef DEBUG
        printf("Error receiving data from socket!\n");
      #endif

      return NULL;
    } else if (bytes == 0) { /* closed connection */

      #ifdef DEBUG
        printf("Connection closed. Got %ld bytes\n", bytesReceived);
      #endif

      (*bodyLength) = bytesReceived;
      return header;
    }

    memcpy((char*)header + (headerLen + bytesReceived), 
           &inputBuffer, bytes);
    bytesReceived += bytes;
  }

  #ifdef DEBUG
    printf("Received %ld\n body bytes\n", bytesReceived);
  #endif

  (*bodyLength) = bytesReceived;
  return header;
}

/* This function is equivalent to recvAll(), except that it makes
 * no attempt to store and return any of the physical data 
 * received over the socket connection.  It only records the number
 * of bytes received over the connection and returns that instead.
 *
 * Like recvAll(), this function can be called from either the client
 * or the server, as checks will be made based on the incoming data
 * to help determine the length of the incoming blocks of information.
 * See the recvAll() documentation for how this is done.
 *
 * The advantage of this function over recvAll() is that it has
 * ONE MEMORY ALLOCATION.  It retrieves the header, and following that,
 * it simply receives data, counts it, and discards it.  If the caller
 * is not interested in doing anything with the data (a la the client), 
 * then this is the function for them.
 *
 * @param socket The socket identifier.
 * @param bodyLength Upon return, this will contain the number of bytes
 *                   in ONLY the body portion of the total bytes.
 * @return The number of bytes received over the socket connection.
 */
long int recvAll_NoData(int socket, long int* bodyLength) {
  long int bLen = 0, hLen = 0;
  char inputBuffer[20000]; /* static input buffer */
  int bytes = 0;
  long int retVal = 0; /* initialize the return value */

  /* nab the header */
  void* header = recvHeader(socket, &hLen, &bLen);

  free(header); /* that's it for memory allocations */

  retVal = hLen; /* add up the header */

  (*bodyLength) = 0; /* set body length to 0 */

  #ifdef DEBUG
    printf("Advertised Content-Length: %ld\n", bLen);
  #endif

  /* keep going with receiving data */
  while ((*bodyLength) < bLen) {
    memset(&inputBuffer, 0, sizeof(inputBuffer));
    bytes = recv(socket, inputBuffer, (sizeof(inputBuffer) - 1), 0);
    if (bytes < 0) { /* error */
      return -1;
    } else if (bytes == 0) { /* closed the connection */
      return retVal;
    }

    /* increment body bytes and total bytes received */
    retVal += bytes;
    (*bodyLength) += bytes;
  }

  /* dat's right. foo. */
  return retVal;
}

/* To make receiving the body easier, this function retrieves only
 * the header, a single byte at a time, and returns that, along 
 * with setting the lengths of both the header and the body so
 * the caller will know exactly what they are dealing with.
 *
 * Using this function also prevents retrieving the header in chunks
 * and possibly splitting the \r\n\r\n delimeter.
 *
 * @param socket The socket identifier from which to receive.
 * @param headerLength Upon function exit, indicates the length in bytes
 *                     of the header that was received.
 * @param bodyLength Upon function exit, indicates the length in bytes
 *                   of the body (as specified by Content-Length). This
 *                   will be 0 if the header was a request from the client.
 * @return A dynamically allocated buffer with the entire header's data
 *         in it.  Its length will be specified by headerLength. Caller
 *         will need to explicitly free() this buffer.
 */
void* recvHeader(int socket, long int* headerLength, long int* bodyLength) {
  char inputChar[5];   /* array of 4 chars */
  char* bLength;       /* will hold Content-Length: xxxxx */
  int bytes = 0;       /* sanity check */
  void* retVal = NULL; /* initialize return value */
  (*headerLength) = 0; /* initialize length */
  (*bodyLength) = 0;   /* initialize body size */

  memset(&inputChar, 0, sizeof(inputChar));

  /* start receiving, one byte at a time */
  do {

    /* move all the bytes down one */
    inputChar[3] = inputChar[2];
    inputChar[2] = inputChar[1];
    inputChar[1] = inputChar[0];
    
    /* receive a byte on the socket ... goes in inputChar[0] */
    bytes = recv(socket, inputChar, sizeof(char), 0);
    if (bytes < 0) { /* error */

      #ifdef DEBUG
        printf("A socket error occurred while receiving the header.\n");
        printf("Bytes received: %ld\n", (*headerLength));
        printf("Header received: %s\n", (char*)retVal);
      #endif

      return NULL;
    } else if (bytes == 0) { /* connection closed */
      return retVal;
    }

    /* reallocate the buffer with enough room */
    retVal = realloc(retVal, sizeof(char) * ((*headerLength) + bytes));
    if (!retVal) {

      #ifdef DEBUG
        printf("Reallocation of header failed.\n");
      #endif

      return NULL;
    }

    /* copy over the new character */
    memcpy((char*)retVal + (*headerLength), &inputChar, bytes);

    /* we have a single character */
    (*headerLength) += bytes;
  } while (!strstr(inputChar, "\n\r\n\r")); /* end of header */

  /* this stumped me for a little while...gotta look for that
   * string in the REVERSE one would normally expect, since I'll be
   * retrieving it one character at a time and shifting it to the right,
   * thereby reversing the string.
   */

  /* now let's hunt for the Content-Length */
  if ((bLength = getHeaderField(retVal, (*headerLength), "Content-Length"))) {
    /* found Content-Length */

    (*bodyLength) = atoi(bLength); /* word */
  } else if (getStatusCode(retVal, (*headerLength)) == 200) {
    /* got an OK from the server, nab that data! */

    (*bodyLength) = -1; /* CHUNKED! */
   
    #ifdef DEBUG
      printf("Trying for arbitrary message-body receive.\n");
    #endif
  }

  /* free the temp string, if needed */
  if (bLength) {
    free(bLength);
  }

  #ifdef DEBUG
    printf("Header Length: %ld\n", (*headerLength));
  #endif

  return retVal;
}

/* This function is identical to recvHeader(), except that instead of reading
 * from a socket, this function will read directly from mapped memory, 
 * returning the header.
 *
 * WARNING: This function makes the (VERY DANGEROUS) assumption that the
 *          ENTIRE header made it into mapped memory!  Yes, this is a very
 *          bad bad bad problem, and it will be corrected in a later release.
 *
 * @param mem A pointer to the mapped block, with the header at the front.
 * @param headerLength Will represent the number of bytes in the header.
 * @param bodyLength Holds the number of bytes in the body, or -1 if unknown.
 * @return A pointer to just the header. This must be freed by the caller!
 */
void* recvHeader_Mem(void* mem, long int* headerLength, long int* bodyLength) {
  char* bLength, *endOfHeader;
  void* retval;
  (*headerLength) = 0;
  (*bodyLength) = 0;

  endOfHeader = strstr((char*)mem, "\r\n\r\n");
  if (!endOfHeader) {
    /* shit. bad bad bad */
    #ifdef DEBUG
      printf("MALFORMED HEADER WRITTEN TO SHARED MEMORY!\n");
    #endif

    return NULL;
  }
  
  /* TODO - What happens if entire header isn't present? */

  /* allocate memory for the header */
  endOfHeader += strlen("\r\n\r\n");
  (*headerLength) = endOfHeader - (char*)mem;
  retval = calloc((*headerLength), sizeof(char));
  if (!retval) {
    #ifdef DEBUG
      printf("recvAll.c: recvHeader_Mem failed to allocate!\n");
    #endif

    return NULL;
  }
  memcpy(retval, mem, (*headerLength));

  /* we have the header, now get the body length and get outta here */
  if ((bLength = getHeaderField(retval, (*headerLength), "Content-Length"))) {
    (*bodyLength) = atoi(bLength);
  } else if (getStatusCode(retval, (*headerLength)) == 200) {
    /* server returned with OK, but didn't supply a content-length? */
    (*bodyLength) = -1;

    #ifdef DEBUG
      printf("recvAll.c: recvHeader_Mem trying for arbitrary receive.\n");
    #endif
  }

  if (bLength) {
    free(bLength);
  }

  #ifdef DEBUG
    printf("recvAll.c: recvHeader_Mem reports header length: %ld\n", (*headerLength));
  #endif

  return retval;
}

/* Similar to recvHeader_Mem, except this leaves the header alone, electing
 * instead to retrieve as much of the body as possible and returning that
 * to the caller.
 *
 * NOTE: This function does take into account the possibility that the
 *       entire body did NOT make it into shared memory.  If this is
 *       the case, the third parameter will be set with how many bytes
 *       out of the total body length were actually retrieved from
 *       memory.
 *
 * @param mem The memory block.
 * @param bodyLength The number of bytes in the body.
 * @param bytesCopied The number of bytes actually returned.
 * @param isHeader A flag to indicate if a header is in the block.
 * @return A pointer to the body.
 */
void* recvBody_Mem(void* mem, long int bodyLength, long int *bytesCopied,
                   int isHeader) {
  char* endOfHeader;
  void* retval;

  /* find the end of the header */
  if (!isHeader) { /* no header! */
    long int retSize;
    if (bodyLength > SEGMENTSIZE) { /* too large */
      retSize = SEGMENTSIZE;
    } else {
      retSize = bodyLength;
    }

    retval = malloc(sizeof(char) * retSize);
    if (!retval) {
      #ifdef DEBUG
        printf("recvAll.c: recvBody_Mem failed to allocate!\n");
      #endif

      return NULL;
    }
    memcpy(retval, mem, retSize);
    (*bytesCopied) = retSize;
    return retval;
  }

  /* a header is present */
  endOfHeader = strstr((char*)mem, "\r\n\r\n");
  if (!endOfHeader) {
    #ifdef DEBUG
      printf("MALFORMED HEADER IN SHARED MEMORY!\n");
    #endif

    return NULL;
  }
  endOfHeader += strlen("\r\n\r\n");

  /* how many bytes made it to shared memory? */
  (*bytesCopied) = SEGMENTSIZE - (endOfHeader - (char*)mem);

  #ifdef DEBUG
    if ((*bytesCopied) < bodyLength) {
      printf("recvAll.c: recvBody_Mem copying partial body.\n");
    }
  #endif

  retval = malloc(sizeof(char) * (*bytesCopied));
  if (!retval) {
    #ifdef DEBUG
      printf("recvAll.c: recvBody_Mem could not allocate memory!\n");
    #endif

    return NULL;
  }
  memcpy(retval, endOfHeader, (*bytesCopied));

  return retval;
}
