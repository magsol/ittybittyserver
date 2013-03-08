#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <xmlrpc-c/base.h>
#include <xmlrpc-c/client.h>
#include "constants.h"

/* global vars */

#define NAME "lollers"
#define VERSION "0.NaN"

/* function headers */

static void dieOnFault(xmlrpc_env* const environment);

/* let's get it on */

int main(int argc, char** argv) {
  xmlrpc_env environment;
  xmlrpc_value* result;
  void* contents;
  size_t elements;
  char serverURL[1000];
  char* methodName = "compress";
  FILE* input;

  unsigned char* compressed;
  int compressedLength;

  /* determine the command line arguments */
  if (argc != 3) {
    printf("Usage:\n\n\t%% ./rpcclient <port> <filename>\n");
    exit(INCORRECT_ARGS);
  }

  /* set up the client */
  xmlrpc_env_init(&environment);
  xmlrpc_client_init2(&environment, XMLRPC_CLIENT_NO_FLAGS, NAME, VERSION, NULL, 0);
  dieOnFault(&environment);

  /* set up the server connection */
  memset(&serverURL, 0, sizeof(serverURL));
  snprintf(serverURL, sizeof(serverURL) - 1, "http://localhost:%d/RPC2", atoi(argv[1]));

  /* read from the image */
  input = fopen(argv[2], "r");
  if (!input) {
    printf("Unable to open file %s for reading.  Exiting...\n", argv[2]);
    exit(INCORRECT_ARGS);
  }
  contents = malloc(sizeof(char));
  elements = 1;
  while (fread((char*)contents + (elements - 1), sizeof(char), 1, input)) {
    contents = realloc(contents, (elements + 1));
    elements++;
  }

  /* format, clean up, and send the request */
  contents = realloc(contents, --elements);
  fclose(input);

  #ifdef DEBUG
    printf("rpcclient: Making XMLRPC call '%s' to server '%s' on file '%s'...\n", methodName, serverURL, argv[2]);
  #endif

  result = xmlrpc_client_call(&environment, serverURL, methodName, "(6)", contents, elements);
  dieOnFault(&environment);

  /* read result */
  xmlrpc_decompose_value(&environment, result, "(6)", &compressed, &compressedLength);
  dieOnFault(&environment);

  if (compressedLength == 0) {
    printf("Error retrieving image from server!\n");
    exit(1);
  }

  printf("Retrieved compressed image.\n");
  printf("Original size: %d\n", elements);
  printf("Compressed size: %d\n", compressedLength);

  xmlrpc_read_int(&environment, result, &compressedLength);
  /* free up result resources */
  xmlrpc_DECREF(result);
  xmlrpc_env_clean(&environment);
  xmlrpc_client_cleanup();

  return SUCCESS;
}

/*
 * Takes care of handling any errors that may come up with the Xmlrpc-c
 * environment.
 */
static void dieOnFault(xmlrpc_env* const environment) {
  if (environment->fault_occurred) {
    printf("XML-RPC Fault: %s (%d)\n", environment->fault_string, environment->fault_code);
    exit(1);
  }
}
