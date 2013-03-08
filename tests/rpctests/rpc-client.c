#include <stdlib.h>
#include <stdio.h>
#include <xmlrpc-c/base.h>
#include <xmlrpc-c/client.h>
/*
#include "config.h"
*/
#define NAME "lollerskates"
#define VERSION "0.NaN"

static void dieIfFaultOccurred(xmlrpc_env* const envP) {
  if (envP->fault_occurred) {
    printf("XML-RPC Fault: %s (%d)\n", envP->fault_string, envP->fault_code);
    exit(1);
  }
}

int main(int argc, char** argv) {

  xmlrpc_env env;
  xmlrpc_value* resultP;
  char* arg1;
  void* contents;
  size_t elems;
  char* serverURL = "http://localhost:8080/RPC2";
  char* methodName = "transfer";
  FILE* input;

  if (argc != 2) {
    printf("Usage:\n\n\t%% ./client <filename>\n");
    exit(1);
  }

  /* error-handling environment */
  xmlrpc_env_init(&env);

  /* starts the client library */
  xmlrpc_client_init2(&env, XMLRPC_CLIENT_NO_FLAGS, NAME, VERSION, NULL, 0);
  dieIfFaultOccurred(&env);

  arg1 = argv[1];

  /* read the file contents */
  input = fopen(arg1, "r");
  if (!input) {
    printf("Unable to open file %s for reading!\n", arg1);
    exit(1);
  }

  contents = malloc(sizeof(char));
  elems = 1;
  while (fread((char*)contents + (elems - 1), sizeof(char), 1, input)) {
    contents = realloc(contents, (elems + 1));
    elems++;
  }

  /* chop off the extra character at the end */
  contents = realloc(contents, --elems);
  fclose(input);
  printf("Making XMLRPC call to server url '%s', method '%s' to send file %s...\n", serverURL, methodName, arg1);

  /* make the procedure call */
  resultP = xmlrpc_client_call(&env, serverURL, methodName, "(6)", contents, elems);
  dieIfFaultOccurred(&env);

  /* retrieve the result */
  xmlrpc_read_int(&env, resultP, &elems);
  dieIfFaultOccurred(&env);
  if (elems) {
    printf("The server encountered an error.\n");
    exit(1);
  } else {
    printf("File successfully transferred!\n");
  }

  /* free up result resources */
  xmlrpc_DECREF(resultP);

  /* free up error handling environment */
  xmlrpc_env_clean(&env);

  /* shut down the client library */
  xmlrpc_client_cleanup();

  printf("Test successful! :)\n");
  return 0;
}
