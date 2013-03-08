#ifndef _XMLRPC_C_SQUINN_
#define _XMLRPC_C_SQUINN_

#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ctype.h>
#include <xmlrpc-c/base.h>
#include <xmlrpc-c/client.h>

#include "sendAll.c"

/* This function facilitates the capabilities of the proxy server
 * by receiving data on one socket and immediately forwarding it
 * on to the next socket.  Given the vast memory requirements that
 * could be needed to create a buffer to store an entire transmission
 * before forwarding it on (a la recvAll()), this function streamlines
 * that process by sending off each data chunk as it is received.
 *
 * @param fromSock The socket identifier on which it receives data.
 * @param toSock The socket identifier to which data is sent.
 * @param header The actual header received.
 * @param headerLength This will contain the length of the header.
 * @param bodyLength This will contain the length of the expected body.
 * @param compression Indicates if incoming file is a JPG.
 * @param env The XML-RPC environment.
 * @param server The RPC server.
 * @return The number of bytes forwarded, or -1 on failure.
 */
int recvAll_Forward(int fromSock, int toSock, void* header,
                    long int headerLength, long int bodyLength,
                    int compression, xmlrpc_env* env, char* server) {
  char inputBuffer[20000];
  long int bytesReceived = 0, bytesSent = 0;
  int arbitraryReceive = 0; /* set this flag if bodyLength < 0 */
  void* imgBuffer, *compImgBuffer;
  long int compImgSize;

  /* first, send out the header */
  if (sendAll(toSock, header, &headerLength) < 0) {
    return -1;
  }

  #ifdef DEBUG
    printf("Length of body to receive: %ld\n", bodyLength);
  #endif

  /* do some checks...is there even a Content-Length? */
  if (bodyLength < 0) { /* going to try... */
    arbitraryReceive = 1;
  }

  /* now, grab the incoming body */
  while (bytesReceived < bodyLength || arbitraryReceive) {
    long int bytes;
    memset(&inputBuffer, 0, sizeof(inputBuffer));
    bytes = recv(fromSock, inputBuffer, (sizeof(inputBuffer) - 1), 0);
    if (bytes < 0) {

      #ifdef DEBUG
        printf("Error receiving data on socket!\n");
      #endif

      return -1;
    } else if (bytes == 0) { /* connection closed */
      if (compression) { /* send it out */
        xmlrpc_value* result;
        char* methodName = "compress";

        /* send RPC request */
        result = xmlrpc_client_call(env, server, methodName, "(6)", imgBuffer, bytesReceived);
        if (env->fault_occurred) {
          #ifdef DEBUG
            printf("proxy.c: Error making sudden RPC client call!\n");
          #endif

          free(compImgBuffer);
          free(imgBuffer);
          return -1;
        }
        xmlrpc_decompose_value(env, result, "(6)", &compImgBuffer, &compImgSize);
        if (env->fault_occurred) {
          #ifdef DEBUG
            printf("proxy.c: Error decomposing sudden RPC server response!\n");
          #endif

          free(compImgBuffer);
          free(imgBuffer);
          xmlrpc_DECREF(result);
          return -1;
        }

        if (sendAll(toSock, compImgBuffer, &compImgSize) < 0) {
          #ifdef DEBUG
            printf("proxy.c: Error sending sudden compressed image to client!\n");
          #endif

          free(compImgBuffer);
          free(imgBuffer);
          xmlrpc_DECREF(result);
          return -1;
        }
        #ifdef DEBUG
          printf("proxy.c: Sent %ld sudden compressed image bytes.\n", bytesSent);
        #endif
        bytesSent = compImgSize;
      }
      return bytesSent + headerLength;
    }

    /* got something valid. now send it back out */
    bytesReceived += bytes;

    if (!compression) { /* forward normally */
      if (sendAll(toSock, inputBuffer, &bytes) < 0) {
        return -1;
      }
    } else { /* need to buffer everything received */
      #ifdef DEBUG
        printf("proxy.c: Buffering - increasing size from %ld to %ld.\n", (bytesReceived - bytes), bytesReceived);
      #endif

      imgBuffer = realloc(imgBuffer, sizeof(char) * bytesReceived);
      memcpy((char*)imgBuffer + (bytesReceived - bytes), inputBuffer, bytes);
    }

    #ifdef DEBUG
      printf("Successfully sent %ld bytes!\n", bytes);
    #endif

    /* success! */
    bytesSent += bytes;
  }

  if (compression && bytesReceived > 0) { /* now we need to send everything */
    xmlrpc_value* result;
    char* methodName = "compress";

    /* send RPC request */
    result = xmlrpc_client_call(env, server, methodName, "(6)", imgBuffer, bytesReceived);
    if (env->fault_occurred) {
      #ifdef DEBUG
        printf("proxy.c: Error making final RPC client call!\n");
      #endif

      free(imgBuffer);
      return -1;
    }
    xmlrpc_decompose_value(env, result, "(6)", &compImgBuffer, &compImgSize);
    if (env->fault_occurred) {
      #ifdef DEBUG
        printf("proxy.c: Error decomposing final RPC server response!\n");
      #endif

      free(imgBuffer);
      return -1;
    }

    /* send the image to the client */
    if (sendAll(toSock, compImgBuffer, &compImgSize) < 0) {
      #ifdef DEBUG
        printf("proxy.c: Error sending final compressed image to client!\n");
      #endif

      free(compImgBuffer);
      free(imgBuffer);
      xmlrpc_DECREF(result);
      return -1;
    }
    bytesSent = compImgSize;

    #ifdef DEBUG
      printf("proxy.c: Image of size %ld compressed to client.\n", bytesSent);
    #endif
  }

  /* return the header length plus number of successful bytes sent */
  return headerLength + bytesSent;
}

#endif /* _XMLRPC_C_SQUINN_ */
