#include <stdlib.h>
#include <stdio.h>
#include <xmlrpc-c/base.h>
#include <xmlrpc-c/server.h>
#include <xmlrpc-c/server_abyss.h>

static xmlrpc_value* transfer(xmlrpc_env* const envP,
                       xmlrpc_value* const paramArrayP,
                       void* const serverInfo) {
  xmlrpc_value* array;
  unsigned char* result;
  size_t resultLength, elemsWritten;
  FILE* output;

  /* arguments come in as arrays; parse them out */
  xmlrpc_decompose_value(envP, paramArrayP, "(6)", &result, &resultLength);
  if (envP->fault_occurred) {
    printf("FAULT!\n");
    return xmlrpc_build_value(envP, "i", 1);
  }

  printf("Bytes: %d\n", resultLength);

  output = fopen("outputfile", "w");
  if (!output) {
    printf("Unable to open file for writing.\n");
    return xmlrpc_build_value(envP, "i", 1);
  }

  if ((elemsWritten = fwrite(result, sizeof(char), resultLength, output)) != resultLength) {
    printf("Error: Only %d of %d bytes written!\n", elemsWritten, resultLength);
    return xmlrpc_build_value(envP, "i", 1);
  }
  fclose(output);
  printf("Write successful!\n");
  return xmlrpc_build_value(envP, "i", 0);
}

int main(int argc, char** argv) {

  xmlrpc_server_abyss_parms serverparm;
  xmlrpc_registry* registryP;
  xmlrpc_env env;

  if (argc != 2) {
    printf("Usage:\n\n\t%% ./server <port>\n");
    exit(1);
  }

  /* do some initialization */
  xmlrpc_env_init(&env);
  registryP = xmlrpc_registry_new(&env);

  /* register the RPC method */
  xmlrpc_registry_add_method(&env, registryP, NULL, "transfer", &transfer, NULL);

  /* set the parameters */
  serverparm.config_file_name = NULL;
  serverparm.registryP = registryP;
  serverparm.port_number = atoi(argv[1]);
  serverparm.log_file_name = "/tmp/xmlrpc_log1";

  printf("XMLRPC server executing...\n");

  /* run the server! */
  xmlrpc_server_abyss(&env, &serverparm, XMLRPC_APSIZE(log_file_name));

  return 0;
}
