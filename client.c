#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "headers/client.h"

/* function headers */

static void* accessServer(void* args);

/* global variables */

pthread_t* threads;		/* pool of testing threads */
pthread_attr_t scope;		/* set system scope scheduling and detached */
int numThreads;			/* run-time parameter indicating pool size */
struct sockaddr_in serveraddr;	/* stores server information */
char* fileList;			/* points to file containing list of files */
char* server;			/* indicates the name of the server */
char* proxy;			/* indicates proxy; could be null */
int port;			/* indicates server port */
int proxyPort;			/* indicates proxy port */

/* Let's get to work! */
int main(int argc, char** argv) {
  int i;
  off_t total;
  struct hostent *he;

  /* check command line argument count */
  /* extract command line arguments */
  if (argc == 5) { /* no proxy */
    server = argv[1];
    port = atoi(argv[2]);
    numThreads = atoi(argv[3]);
    fileList = argv[4];
    proxy = NULL;
  } else if (argc == 8) { /* proxy! */
    proxy = argv[2];
    proxyPort = atoi(argv[3]);
    server = argv[4];
    port = atoi(argv[5]);
    numThreads = atoi(argv[6]);
    fileList = argv[7];
  } else { /* fail */
    printArgs(CLIENT);
    exit(INCORRECT_ARGS);
  }

  /* create thread pool */
  if ((threads = malloc(sizeof(pthread_t) * numThreads)) == NULL) {
    printf("Error allocating memory for threads.  Exiting...\n");
    exit(MEMALLOC_FAILURE);
  }

  /* initialize server reference */
  if ((he = gethostbyname((proxy ? proxy : server))) == NULL) {
    printf("Error retrieving host name.  Exiting...\n");
    exit(INCORRECT_ARGS);
  }
  memset(&serveraddr, 0, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_port = htons((proxy ? proxyPort : port));
  serveraddr.sin_addr = *((struct in_addr*)he->h_addr);

  /* initialize the thread attribute and mutex */
  pthread_attr_init(&scope);
  pthread_attr_setscope(&scope, PTHREAD_SCOPE_SYSTEM);
  pthread_attr_setdetachstate(&scope, PTHREAD_CREATE_JOINABLE);

  /* start threads! */
  for (i = 0; i < numThreads; i++) {
    pthread_create(&threads[i], &scope, accessServer, (void*)i);

    #ifdef DEBUG
      printf("Thread %d started!\n", i);
    #endif
  }

  /* now wait for threads to finish */
  total = 0;
  for (i = 0; i < numThreads; i++) {
    void* retVal;
    if (pthread_join(threads[i], &retVal)) {
      printf("Error joining thread %d.  Exiting...\n", i);
      exit(MUTEX_FAILURE);
    }

    #ifdef DEBUG
      printf("Thread %d joined!\n", i);
    #endif

    total += (long)retVal;
  }

  /* clean up */
  free(threads);
  pthread_attr_destroy(&scope);

  /* print out total bytes received */
  printf("Total number of bytes received from server: %ld\n", total);
  return SUCCESS;
}

/* This function will access the server, retrieve a bunch of files at
 * random, and record the number of bytes received from the server
 * and return that number
 */
static void* accessServer(void* args) {
  FILE* fp;
  int remoteSocket, numFiles, randNum;
  long int receive;
  char** completeList = NULL;
  char buffer[10000];
  int ID = (int)args;
  long int retVal = 0;
  unsigned int state = 1;
  int i;

  #ifdef DEBUG
    printf("Obtaining file listings.\n");
  #endif

  /* open up the file to get the list of files */
  if ((fp = fopen(fileList, "r")) == NULL) {
    printf("Error opening file %s.  Exiting...\n", fileList);
    exit(IO_FAILURE);
  }

  /* read in the file names */
  numFiles = 0;
  memset(&buffer, 0, sizeof(buffer));
  while (fgets(buffer, (sizeof(buffer) - 1), fp)) {

    char* newline = strchr(buffer, '\n');
    if (newline) {
      newline[0] = '\0'; /* chop off newline */
    }
    if (buffer[0] == '\n' || buffer[0] == ' ') { /* empty */
      break;
    }

    #ifdef DEBUG
      printf("File %s loaded.\n", buffer);
    #endif

    numFiles++;
    completeList = realloc(completeList, sizeof(char*) * numFiles);
    completeList[numFiles - 1] = strdup(buffer);
    memset(&buffer, 0, sizeof(buffer));
  }
  fclose(fp);

  /* access server */
  for (i = 0; i < NUMACCESSES; i++) {
    char transfer[20000];
    long int requestLen;
    long int bodyLen = 0;

    memset(&transfer, 0, sizeof(transfer));
    randNum = rand_r(&state) % numFiles;

    /* determine whether we are sending to a server or a proxy */
    if (proxy) { /* proxy */
      snprintf(transfer, (sizeof(transfer) - 1),
               "GET %s:%d%s%s HTTP/1.0%sHost: %s:%d%s%s", server, port, 
               (completeList[randNum][0] == '/' ? "" : "/"),
               completeList[randNum], 
               EOL, (server + 7), port, EOL, EOL);

      #ifdef DEBUG
        printf("Thread %d: Establishing contact with proxy.\n", ID);
      #endif
    } else { /* server */
      snprintf(transfer, (sizeof(transfer) - 1), 
              "GET /%s HTTP/1.0%sHost: %s%s%s",
               completeList[randNum], EOL, server, EOL, EOL);
 
      #ifdef DEBUG
        printf("Thread %d: Establishing contact with server.\n", ID);
      #endif
    }
 
    /* establish the connection with the server */
    if ((remoteSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
      printf("Error opening socket.  Exiting...\n");
      exit(SOCKET_FAILURE);
    }

    /* establish the connection */
    if (connect(remoteSocket, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) {
      printf("Error establishing connection with server.  Exiting...\n");
      exit(NETWORK_FAILURE);
    }

    #ifdef DEBUG
      printf("Connection established, sending request.\n");
    #endif

    /* send the request! */
    requestLen = strlen(transfer);
    if (sendAll(remoteSocket, transfer, &requestLen) < 0) {
      printf("Error sending request to server.  Exiting...\n");
      exit(NETWORK_FAILURE);
    }
    memset(&transfer, 0, sizeof(transfer));

    #ifdef DEBUG
      printf("Request sent, awaiting server response.\n");
    #endif

    /* wait on the response */
    if ((receive = recvAll_NoData(remoteSocket, &bodyLen)) < 0) {
      printf("Error receiving response from server.  Exiting...\n");
      exit(NETWORK_FAILURE);
    }
    retVal += bodyLen;
    
    #ifdef DEBUG
      printf("Thread: %d | Files: %s | Bytes: %ld\n", ID, completeList[randNum], bodyLen);
    #endif
  } 

  /* clean up */
  for (i = 0; i < numFiles; i++) {
    free(completeList[i]);
  }
  free(completeList);
  close(remoteSocket);

  /* return */
  return (void*)retVal;
}
