#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <xmlrpc-c/base.h>
#include <xmlrpc-c/server.h>
#include <xmlrpc-c/server_abyss.h>
#include "lowres.h"
#include "constants.h"

/* global variables */

char* filename = "images/image";
static int imageNumber = 0;

/* function headers */

static xmlrpc_value* compress(xmlrpc_env* const environment,
                              xmlrpc_value* const paramArray,
                              void* const serverInfo);

/* let's get it on */

int main(int argc, char** argv) {
  xmlrpc_server_abyss_parms serverparm;
  xmlrpc_registry* registryP;
  xmlrpc_env environment;

  /* first, check the command-line arguments */
  if (argc != 2) {
    printf("Usage:\n\n\t%% ./rpcserver <port>\n");
    exit(INCORRECT_ARGS);
  }
  
  /* establish the RPC server */
  xmlrpc_env_init(&environment);
  registryP = xmlrpc_registry_new(&environment);
  xmlrpc_registry_add_method(&environment, registryP, NULL, "compress", &compress, NULL);
  serverparm.config_file_name = NULL;
  serverparm.registryP = registryP;
  serverparm.port_number = atoi(argv[1]);
  serverparm.log_file_name = "xmlrpc-c_log";

  #ifdef DEBUG
    printf("XML-RPC server listening on port %d...\n", atoi(argv[1]));
  #endif

  /* run the server indefinitely! */
  xmlrpc_server_abyss(&environment, &serverparm, XMLRPC_APSIZE(log_file_name));

  return SUCCESS;
}

/* This function provides the functionality to compress images
 * them.  A temporary file is created in order to use a file descriptor
 * to read from it and pass that information through the compression
 * algorithm.  After its use, the temporary file is deleted.
 *
 * If DEBUG mode is enabled, the temporary file (the original) and the 
 * compressed image both remain in the system and must be deleted manually.
 *
 * @param environment The Xmlrpc-c environment variable.
 * @param paramArray The XML array of parameters from the client.
 * @param serverInfo The state of this XML-RPC server
 * @return An RPC transmission containing the compressed image data. If
 *         an error occurs, the length of this transmission will be 0.
 */
static xmlrpc_value* compress(xmlrpc_env* const environment,
                              xmlrpc_value* const paramArray,
                              void* const serverInfo) {

  unsigned char* result;
  char* compressed;
  size_t length;
  int compressedLength;
  FILE* file;
  int fd, written;
  char name[1000];
  int id = imageNumber++;

  /* parse out the incoming arguments */
  xmlrpc_decompose_value(environment, paramArray, "(6)", &result, &length);
  if (environment->fault_occurred) {
    printf("Error: Unable to decompose argument array!\n");
    return xmlrpc_build_value(environment, "(6)", NULL, 0);
  }

  #ifdef DEBUG
    printf("rpcserver.c: Received image of size %d.\n", length);
  #endif

  /* now we have the image we'd like to compress */
  memset(&name, 0, sizeof(name));
  snprintf(name, sizeof(name) - 1, "%s%d.jpg", filename, id);

  /* write the data to a temporary file */
  file = fopen(name, "w");
  if (!file) {
    printf("Error: Unable to create temporary file!\n");
    return xmlrpc_build_value(environment, "(6)", NULL, 0);
  }
  if ((written = fwrite(result, sizeof(char), length, file)) != length) {
    printf("Error: Only %d of %d bytes written!\n", written, length);
    return xmlrpc_build_value(environment, "(6)", NULL, 0);
  }
  fclose(file);

  /* we have the temporary file; open it and pass its file descriptor
   * to the compression algorithm */
  fd = open(name, O_RDWR);
  if (fd < 0) {
    printf("Error: Unable to open file for compressing!\n");
    return xmlrpc_build_value(environment, "(6)", NULL, 0);
  }
  if (change_res_JPEG(fd, &compressed, &compressedLength) == 0) {
    printf("Error: Unable to compress image!\n");
    return xmlrpc_build_value(environment, "(6)", NULL, 0);
  }
  close(fd);    

  /* compression successful! */
  #ifndef DEBUG
    unlink(name);
  #else
    {
      FILE* cImage;
      char compression[1000];
      memset(&compression, 0, sizeof(compression));
      snprintf(compression, sizeof(compression) - 1, "%s_compressed.jpg", name);
      cImage = fopen(compression, "w");
      if (!cImage) {
        printf("rpcserver.c: Unable to open file %s for writing.\n", compression);
      } else {
        int cWrite = fwrite(compressed, sizeof(char), compressedLength, cImage);
        if (cWrite != compressedLength) {
          printf("rpcserver.c: Only %d of %d of compressed bytes written.\n", cWrite, compressedLength);
        } else {
          printf("rpcserver.c: Original (%d) and compressed (%d) images written to file system.\n", length, compressedLength);
        }
      }
      fclose(cImage);
    }
  #endif

  /* successful compression!  return the compressed image */
  return xmlrpc_build_value(environment, "(6)", (unsigned char*)compressed, compressedLength);
}
