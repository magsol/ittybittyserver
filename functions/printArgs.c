#ifndef _PRINTARGS_
#define _PRINTARGS_

#include "../headers/constants.h" /* for program type */

/* Given a program type, this function simply prints the usage
 * syntax for that program to the console.
 *
 * @param program The program type calling this function.
 */
void printArgs(int program) {

  switch (program) {

    case CLIENT:
      printf("Squinn Client\n\nUsage:\n\t%% ./client [-p <proxy> <proxy port>] <server> <server port> <# of threads> <file library>\n\n");
      printf("          -p : Optional argument, implies use of proxy.\n");
      printf("       proxy : Proxy server to use (REQUIRES -p).\n");
      printf("  proxy port : Proxy server port to connect to (REQUIRES -p).\n");
      printf("      server : Server to request files from (if -p is specified, this MUST be a full pathname).\n");
      printf(" server port : Server port to connect to.\n");
      printf("# of threads : Number of client threads to access server with.\n");
      printf("file library : List of files on the server to request.\n");
      
      break;

    case SERVER:
      printf("%s %s\n\nUsage:\n\t%% ./server <listening port> <# of threads> [directory] [-o]\n\n", SERVER_NAME, VERSION);
      printf("listening port : Listens here for client connections.\n");
      printf("  # of threads : Number of worker threads to handle connections.\n");
      printf("     directory : Optional server document root.\n");
      printf("            -o : Server is optimized for shared memory use.\n");
      break;

    case PROXY:
      printf("Squinn Proxy\n\nUsage:\n\t%% ./proxy <listening port> <# of threads> [-c <RPC server> <port>] [-o]\n\n");
      printf("listening port : Listens here for client connections.\n");
      printf("  # of threads : Number of worker threads to handle connections.\n");
      printf("            -c : Proxy will compress JPG images it receives.\n");
      printf("   dist server : Distributed server proxy will send JPGs for compression.\n");
      printf("          port : Port number for the distributed server.\n");
      printf("            -o : Proxy is optimized for shared memory use.\n");
      break;
  }
}

#endif /* _PRINTARGS_ */
