#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <netdb.h>
#include <errno.h>

#include "headers/proxy.h"

/* function headers */

static void initializeGlobals(void);
static void cleanUpGlobals(void);
static void catchInterrupt(int signum);
static void* handleClient(void* args);
static int sharedProxy(const char* server, struct hostent* h, 
                       connection* client, void* header,
                       long int headerLength, int compression, 
                       xmlrpc_env* environment);
static int isCompressed(int numargs, char** arguments);
static int rpcFault(xmlrpc_env* const environment);

/* global variables */

pthread_t* workers;		/* pool of worker threads */
pthread_attr_t scope;		/* set system scope for thread scheduling */
pthread_mutex_t mConList;	/* protects connections list */
pthread_cond_t freeConn;	/* signaled when a connection is ready */
conlist* list;			/* list of client connections */
int numThreads;			/* number of proxy threads */
int serverSock;			/* TRANSMITS and LISTENS to SERVER */
int clientSock;			/* TRANSMITS to CLIENTS */
int LOOP;			/* infinite main loop */
int OPTIMIZED;			/* is this proxy optimized? */
int COMPRESS;			/* are we compressing images? */
char* distserver;		/* distributed image compression server */
int distport;			/* distributed image compression port */
memnode* shList;		/* shared memory list */
metanode* shMeta;		/* shared metanode */
xmlrpc_env environment;		/* the RPC environment */

/* Let's get started! */

int main(int argc, char** argv) {
  int listenSock;		/* LISTENS for CLIENTS */
  struct sockaddr_in localaddr;	/* local address struct */
  struct sockaddr_in clientaddr;/* client address struct */
  struct sigaction sa;		/* traps signals */
  int i;

  /* check command line arguments */
  if (argc < 3 || argc > 7) {
    printArgs(PROXY);
    exit(INCORRECT_ARGS);
  }

  /* optimization? */
  OPTIMIZED = isOptimized(argc, argv);

  /* image compression? */
  if ((COMPRESS = isCompressed(argc, argv))) {
    distserver = argv[4];
    if (strstr(distserver, "http://")) {
      printf("Error: Distributed server name should not contain protocol.\n");
      printArgs(PROXY);
      exit(INCORRECT_ARGS);
    }
    distport = atoi(argv[5]);
  }

  numThreads = atoi(argv[2]);
  if (numThreads <= 0) { /* yeah, user is basically an idiot */
    printf("Invalid thread count \"%d\".  Exiting...\n", numThreads);
    exit(INCORRECT_ARGS);
  }

  /*****************\
  * Initializations *
  \*****************/

  /* loop! */
  LOOP = 1;

  /* initialize listening socket */
  memset(&localaddr, 0, sizeof(localaddr));
  localaddr.sin_family		= AF_INET;
  localaddr.sin_addr.s_addr	= htonl(INADDR_ANY);
  localaddr.sin_port		= htons(atoi(argv[1]));

  /* create listening socket */
  if ((listenSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    printf("Error opening socket for listening.  Exiting...\n");
    exit(SOCKET_FAILURE);
  }

  /* bind listening socket */
  if (bind(listenSock, (struct sockaddr *) &localaddr, sizeof(localaddr)) < 0) {
    printf("Error binding port to local address.  Exiting...\n");
    exit(SOCKET_FAILURE);
  }

  /* set up to listen */
  if (listen(listenSock, MAXCONNECTIONS_PROXY) < 0) {
    printf("Error listening for incoming connections.  Exiting...\n");
    exit(SOCKET_FAILURE);
  }

  /* initialize interrupt handler */
  sa.sa_handler = catchInterrupt;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGINT, &sa, NULL);

  /* set up the global variables */
  initializeGlobals();

  /*************************************\
  |* Thread creation and infinite loop *|
  \*************************************/

  /* allocate thread memory */
  for (i = 0; i < numThreads; i++) {
    pthread_create(&workers[i], &scope, handleClient, (void *)i);
  }

  /* loop until the interrupt handler changes this value */
  while (LOOP) {
    unsigned int clientLength = sizeof(clientaddr);

    #ifdef DEBUG
      printf("Waiting for an incoming connection...\n");
    #endif

    if ((clientSock = accept(listenSock, (struct sockaddr *) &clientaddr, &clientLength)) < 0) {
      if (!LOOP) { /* loop was broken, don't worry 'bout it */
        break;
      }

      /* ok, got here not because of LOOP, but an actual error */
      printf("Error accepting incoming connection.  Exiting...\n");
      exit(SOCKET_FAILURE);
    } else {

      #ifdef DEBUG
        printf("Proxy: Connection from %s\n", inet_ntoa(clientaddr.sin_addr));
      #endif

      /* add the new connection */
      pthread_mutex_lock(&mConList);
      addTail(clientSock, PROCESS, list);
      pthread_cond_broadcast(&freeConn);
      pthread_mutex_unlock(&mConList);
    }
    /* keep on truggin' */
  }
  
  /* clean things up */
  cleanUpGlobals();
  
  /* close the socket */
  close(listenSock);

  printf("Proxy service terminated! :)\n");

  return SUCCESS;
}

/* This is the helper function that all threads execute.  Within this
 * function, threads will loop until a connection is made available 
 * for them to service. 
 */
static void* handleClient(void* args) {
  connection* client;
  struct hostent *he, *hp;
  struct sockaddr_in serveraddr;
  char buf[1000];
  int error;
  int ID = (int)args;
  xmlrpc_env environment;
  char serverURL[1000];

  #ifdef DEBUG
    printf("Thread %d starting!\n", ID);
  #endif

  /* set up RPC environment */
  if (COMPRESS) {
    xmlrpc_env_init(&environment);
    xmlrpc_client_init2(&environment, XMLRPC_CLIENT_NO_FLAGS, "Xmlrpc-c Client", "3.0", NULL, 0);
    rpcFault(&environment);
    memset(&serverURL, 0, sizeof(serverURL));
    snprintf(serverURL, sizeof(serverURL) - 1, "http://%s:%d/RPC2", distserver, distport);
  }

  while (1) { /* loop until forever */
    void* header, *tHeader;
    char* fullServer, *uriServer;
    long int headerLen = 0;
    long int bodyLen = 0;
    int bytes;
    int compression;

    /* wait until a connection makes itself available */
    pthread_mutex_lock(&mConList);
    while (isEmpty(list)) {
      pthread_cond_wait(&freeConn, &mConList);
    }
    client = removeHead(list);
    pthread_mutex_unlock(&mConList);

    #ifdef DEBUG
      if (client->action == PROCESS) {
        printf("Thread %d given a client.\n", ID);
      } else {
        printf("Thread %d beginning termination.\n", ID);
      }
    #endif

    if (client->action == TERMINATE) { /* time to quit */
      free(client);

      if (COMPRESS) {
        xmlrpc_env_clean(&environment);
        xmlrpc_client_cleanup();
      }

      #ifdef DEBUG
        printf("Thread %d terminated.\n", ID);
      #endif

      pthread_exit(0);
    }

    /* by getting here, we have a connection to process */
    header = recvHeader(client->conn, &headerLen, &bodyLen);
    if (!header) { /* badness */

      #ifdef DEBUG
        printf("Thread %d: Null header received!\n", ID);
      #endif

      sendError(500, "Internal Server Error", (char*)0, "The proxy encountered an error.", client, 0);
      close(client->conn);
      free(client);
      continue;
    }

    #ifdef DEBUG
      printf("Thread %d: Received header of length %ld\n", ID, headerLen);
    #endif

    /* sets whether the server response will be compressed */
    compression = (COMPRESS && isJPG(header, headerLen) ? 1 : 0);

    /* extract the destination server */
    fullServer = getHeaderField(header, headerLen, "\nHost");
    if (!fullServer) { /* oy */

      #ifdef DEBUG
        printf("Thread %d: Unable to extract hostname from header!\n", ID);
      #endif

      sendError(500, "Internal Server Error", (char*)0, "The proxy encountered an error.", client, 0);
      close(client->conn);
      free(header);
      free(client);
      continue;
    }

    #ifdef DEBUG
      printf("Thread %d: Server at \"%s\"\n", ID, fullServer);
    #endif
    
    #ifdef DEBUG
      {
        char* sample = calloc(headerLen + 1, sizeof(char));
        if (!sample) { printf("FAILURE!!!"); exit(1); }
        memcpy(sample, (char*)header, headerLen);
        printf("Thread %d:\n---ORIG request start---\n%s|\n---ORIG request end---\n", ID, sample);
        free(sample);
      }
    #endif
    
    /* get just the host name */
    uriServer = chopPortNum(fullServer);

    /* allocate the host entity */
    if ((he = calloc(1, sizeof(struct hostent))) == NULL) {
      printf("Memory allocation error.  Continuing.\n");
      sendError(500, "Internal Server Error", (char*)0, "The proxy has encountered an error.\n", client, 0);
      close(client->conn);
      free(client);
      free(uriServer);
      free(fullServer);
      free(header);
      continue;
    }

    /* set up the connection to the server */
    if ((gethostbyname_r(uriServer, he, buf, sizeof(buf) - 1, &hp, &error)) != 0) {
      printf("Error retrieving host name for \"%s\". Skipping.\n", uriServer);
      sendError(404, "Not Found", (char*)0, "Server not found.\n", client, 0);
      close(client->conn);
      free(client);
      free(uriServer);
      free(fullServer);
      free(header);
      continue;
    }

    /* ---=SHARED MEMORY=--- */
    /* usage successful! no further processing needed */
    if (sharedProxy(uriServer, he, client, header, headerLen, compression, &environment) == 0) {

      /* free up resources, close socket */
      free(uriServer);
      free(fullServer);
      free(he);
      /*free(header);*/
      close(client->conn);
      free(client);

      #ifdef DEBUG
        printf("proxy.c: Shared memory request completed by thread %d!\n", ID);
      #endif

      /* move along, move along */
      continue;
    }

    /* ---=END SHARED MEMORY=--- */

    /* set up the server struct */
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port   = htons(getPortNumber(fullServer));
    serveraddr.sin_addr   = *((struct in_addr *)he->h_addr);

    /* don't need references to the server anymore */
    free(uriServer);
    free(fullServer);
    free(he);

    /* set up the socket with the actual web server */
    if ((serverSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
      printf("Error opening server socket.  Skipping.\n");
      sendError(500, "Internal Server Error", (char*)0, "The proxy encountered an error.\n", client, 0);
      close(client->conn);
      free(header);
      free(client);
      continue;
    }

    /* establish the connection */
    if (connect(serverSock, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) {
      printf("Error establishing connection with server.  Skipping.\n");
      sendError(408, "Request Timeout", (char*)0, "The server did not respond to proxy requests.\n", client, 0);
      close(client->conn);
      close(serverSock);
      free(header);
      free(client);
      continue;
    }

    #ifdef DEBUG
      printf("Thread %d: Established connection with server.\n", ID);
    #endif
    

    /* strip out the absolute URL */
    tHeader = stripAbsURL(header, headerLen, &headerLen);
    if (!tHeader) { /* ughhhhhhhasdjkfhasdfj */
      printf("Error stripping out absolute URL from header.  Skipping.\n");
      sendError(500, "Internal Server Error", (char*)0, "The proxy encountered an error.\n", client, 0);
      close(client->conn);
      close(serverSock);
      free(client);
      continue;
    }

    /* free the former header and reassign it to the new one */
    free(header);
    header = tHeader;

    #ifdef DEBUG
      {
        char* sample = calloc(headerLen + 1, sizeof(char));
        if (!sample) { printf("FAILED!!!"); exit(1); }
        memcpy(sample, (char*)header, headerLen);
        printf("Thread %d:\n---request start---\n%s|\n---request end---\n", ID, sample);
        free(sample);
      }
    #endif

    /* send the client header */
    if (sendAll(serverSock, header, &headerLen) < 0) { /* doh */
 
      #ifdef DEBUG
        printf("Thread %d: ", ID);
      #endif

      printf("Error sending header to server.  Skipping.\n");
      sendError(500, "Internal Server Error", (char*)0, "The proxy encountered an error.\n", client, 0);
      close(client->conn);
      close(serverSock);
      free(client);
      free(header);
      continue;
    }

    #ifdef DEBUG
      printf("Thread %d: Header sent, awaiting server response.\n", ID);
    #endif

    /* receive the server's response, WITH the body */
    free(tHeader);
    header = recvHeader(serverSock, &headerLen, &bodyLen);

    if (!header) { /* christ */

      #ifdef DEBUG
        printf("Thread %d: ", ID);
      #endif

      printf("No header received.  Skipping.\n");
      sendError(500, "Internal Server Error", (char*)0, "The proxy encountered an error.\n", client, 0);
      close(client->conn);
      close(serverSock);
      free(client);
      continue;
    }

    #ifdef DEBUG
    {
      char* sample = calloc(headerLen + 1, sizeof(char));
      if (!sample) { /* just quit */ printf("EXITING"); exit(1); }
      printf("Thread %d: Received header from server.\n", ID);
      if (!header) { printf("\n\nFOOLED YOU!!!\n\n"); }
      memcpy(sample, (char*)header, headerLen);
      printf("Thread %d:\n---response start---\n%s|\n---response end---\n", ID, sample);
      free(sample);
    }
    #endif

    if ((bytes = recvAll_Forward(serverSock, client->conn, header, 
                                 headerLen, bodyLen, 
                                 compression, &environment, serverURL)) < 0) {
      printf("Error forwarding server response to client.  Skipping.\n");
      sendError(500, "Internal Server Error", (char*)0, "The proxy encountered an error.\n", client, 0);
      close(client->conn);
      close(serverSock);
      free(client);
      free(header);
      continue;
    }

    #ifdef DEBUG
      printf("Thread %d: %d bytes forwarded!\n", ID, bytes);
    #endif

    /* that should be it!  free the resources */
    close(serverSock);
    close(client->conn);
    free(client);
    free(header);
    
  } /* end infinite loop */

}

/* A utility function that simply initializes the global variables
 * and readies them for use by threads.
 */
static void initializeGlobals(void) {

  /* initialize mutex */
  if (pthread_mutex_init(&mConList, NULL) != 0) {
    printf("Error initializing mutex.  Exiting...\n");
    exit(MUTEX_FAILURE);
  }

  /* initialize condition variable */
  if (pthread_cond_init(&freeConn, NULL) != 0) {
    printf("Error initializing condition variable.  Exiting...\n");
    exit(MUTEX_FAILURE);
  }

  /* allocate worker pool */
  if (!(workers = malloc(sizeof(pthread_t) * numThreads))) {
    printf("Error allocating memory for thread pool.  Exiting...\n");
    exit(MEMALLOC_FAILURE);
  }

  /* set up thread attributes */
  pthread_attr_init(&scope);
  pthread_attr_setscope(&scope, PTHREAD_SCOPE_SYSTEM);
  pthread_attr_setdetachstate(&scope, PTHREAD_CREATE_JOINABLE);

  /* set up connections list */
  list = newConList();
  if (!list) {
    printf("Error allocating memory for connections list.  Exiting...\n");
    exit(MEMALLOC_FAILURE);
  }

  /* shared memory optimization */
  if (OPTIMIZED) {
    shMeta = getMetanode();
    if (!shMeta) {
      printf("Error allocating shared metanode.  Exiting...\n");
      exit(MEMALLOC_FAILURE);
    }
    if (shMeta->serverFlag == OFFLINE) { /* server's not up */
      shMeta->numNodes = NUMSEGMENTS;
    }
    shList = getMemList(shMeta->numNodes);

    if (!shList) {
      printf("Error creating shared memory blocks.  Exiting...\n");
      exit(MEMALLOC_FAILURE);
    }

    /* set the proxy to online */
    shMeta->proxyFlag = ONLINE;
  }


  /* that's about all we can do from here */
}

/* Catches and handles clean-up with SIGINT
 */
static void catchInterrupt(int signum) {
  int i;

  #ifdef DEBUG
    if (LOOP) {
      printf("SIGINT caught, terminating loop...\n");
    } else {
      printf("SIGINT caught, but interrupt already occurring!\n");
    }
  #endif

  /* to prevent someone from hitting CTRL + C multiple times... */
  if (!LOOP) {
    printf("SIGINT already caught - kill process manually if it is hanging.\n");
    return;
  }

  LOOP = 0; /* terminates the infinite loop listening for clients */
  for (i = 0; i < numThreads; i++) {
    /* add termination tokens */
    pthread_mutex_lock(&mConList);
    addTail(0, TERMINATE, list);
    pthread_cond_broadcast(&freeConn);
    pthread_mutex_unlock(&mConList);
  }

  #ifdef DEBUG
    printf("All termination tokens added.  Waiting for exit...\n");
  #endif

  /* let the termination tokens and main() take it from here */
}

/* This will deallocate the memory allocated by the global variables.
 */
static void cleanUpGlobals(void) {
  int i;
  void* status;

  /* first, clean up the threads so the mutex doesn't lock itself */
  for (i = 0; i < numThreads; i++) {
    int retval = pthread_join(workers[i], &status);
    if (retval) { /* joined with an error */
      printf("Error joining thread %d. Code: %d\n", i, retval);
    }

    #ifdef DEBUG
      printf("Thread %d successfully joined! Status: %ld\n", i, (long)status);
    #endif
  }

  /* threads are joined, now destroy the mutex */
  if (pthread_mutex_destroy(&mConList) != 0) {
    printf("Error destroying connection mutex!\n");
  }
  #ifdef DEBUG
    else {
      printf("Connection mutex successfully destroyed.\n");
  }
  #endif

  /* destory condition variable */
  if (pthread_cond_destroy(&freeConn) != 0) {
    printf("Error destroying condition variable!\n");
  }
  #ifdef DEBUG
    else {
      printf("Condition variable successfully destroyed.\n");
  }
  #endif

  /* destroy thread attributes */
  if (pthread_attr_destroy(&scope) != 0) {
    printf("Error destroying thread attributes!\n");
  }
  #ifdef DEBUG
    else {
      printf("Thread attributes successfully destroyed.\n");
  }
  #endif

  /* destroy connections list */
  destroyAll(list);
  free(list);

  /* destroy list of workers */
  free(workers);

  /* shared memory? */
  /* normally, it should be the server's job to eliminate the shared
   * memory hunks, since in all probability it will exit before the 
   * proxy does.  However, a check is made to see if the server has
   * exited prematurely, but left the shared memory in place.  If this
   * is the case, the proxy will erase it.
   */
  if (OPTIMIZED && shMeta->serverFlag == OFFLINE) {
    if (destroyMemList(shList, shMeta->numNodes) < 0) {
      printf("Failure destroying shared memory list!\n");
    }

    if (destroyMetanode(shMeta) < 0) {
      printf("Failure destroying metanode!\n");
    }
  }

  /* everything else is up to main() */
}

/* This function performs the necessary operations to conduct
 * communication over shared memory with a server.  Checks are
 * made throughout to determine whether this transfer can be
 * successfully done.  If not, the function returns with an error
 * flag set to indicate that processing should be conducted over
 * sockets.
 *
 * To protect against an "accidental" error, in which the proxy 
 * might fail in this function even though the server is ready and
 * waiting to accept shared memory requests, no writes to shared memory
 * are made until there is absolutely no doubt that a response will
 * come from the server in the case that the request is written to
 * shared memory.
 *
 * @param server The IP address of the server.
 * @param h The host entity of the server.
 * @param client The socket connection to the client.
 * @param header The header received from the client.
 * @param headerLength The length, in bytes, of the header.
 * @param compression Flag indicating whether this request is for a JPG.
 * @param environment The XMLRPC environment for this thread.
 * @return -1 on failure, 0 on success.
 */
static int sharedProxy(const char* server, struct hostent* h, 
                       connection* client, void* header,
                       long int headerLength, int compression,
                       xmlrpc_env* environment) {
  char* ip;
  char buf[100], buffer[1000];
  struct hostent* he, *hp;
  memnode* node;
  int error;
  long int bytes, compImgLen;
  void* response, *tHeader, *imageBuffer, *compImg;

  /* sanity check */
  if (!OPTIMIZED) {
    return -1;
  } 

  /* now make checks to see if we're on the same machine as the server */
  memset(&buffer, 0, sizeof(buffer));
  he = calloc(1, sizeof(struct hostent));
  if (!he) { /* dammit */
    #ifdef DEBUG
      printf("proxy.c: calloc() error, unable to service shmem request.\n");
    #endif

    return -1;
  }
  gethostname(buf, sizeof(buf) - 1);
  if (gethostbyname_r(buf, he, buffer, sizeof(buffer) - 1, &hp, &error) != 0) {
    #ifdef DEBUG
      printf("proxy.c: gethostbyname_r failure.\n");
    #endif

    free(he);
    return -1;
  }
  ip = inet_ntoa(*((struct in_addr *)he->h_addr));
  free(he);

  if (strcmp(ip, inet_ntoa(*((struct in_addr *)h->h_addr))) != 0) { /* compare IPs */

    #ifdef DEBUG
      printf("\"%s\" != \"%s\"\n", ip, inet_ntoa(*((struct in_addr *)h->h_addr)));
    #endif

    return -1;
  }

  /* finally, is the local server even optimized? */
  if (shMeta->serverFlag == OFFLINE) {

    #ifdef DEBUG
      printf("proxy.c: serverFlag is OFFLINE, or no shmem nodes.\n");
    #endif

    return -1;
  }

  /* getting to this point, we assume that the server and proxy are 
   * ready and waiting with request/response capability over shared
   * memory...get to it!
   */ 

  /* first, get an idle node */
  node = findState(shList, shMeta->numNodes, IDLE);
  if (!node) { /* recockulous */
    #ifdef DEBUG
      printf("proxy.c: No IDLE shared nodes available, using sockets.\n");
    #endif

    return -1;
  }

  /* tell the server we have a request */
  pthread_mutex_lock(&(node->mutex));
  node->proxyState = WAITING_INIT_SRVR;

  /* wait until the server sees the request */
  if (node->serverState != WAITING_CONT_PRXY) {
    pthread_cond_wait(&(node->condition), &(node->mutex));   
  }

  /* strip out the absolute URL */
  tHeader = stripAbsURL(header, headerLength, &headerLength);
  #ifdef DEBUG
    printf("--NEW HEADER--\n%s|\n--END--\n", (char*)tHeader);
  #endif
  free(header);
  header = tHeader;

  /* now map the request */
  sendShared(header, headerLength, NULL, 0, node);

  /* don't need this anymore */
  free(header);

  /* wait on the server to read the information */
  /* pthread_cond_wait(&(node->condition), &(node->mutex)); */

  /* now, receive the incoming server response */
  do {

    /* wait for server to signal */
    if (node->serverState != COMPLETE) {
      pthread_cond_wait(&(node->condition), &(node->mutex));
    }

    /* once the response arrives, read the data and forward it */
    if ((response = receiveShared(node)) == NULL) {
      printf("FATAL PROXY ERROR: UNABLE TO READ SHARED MEMORY!\n");
      exit(MEMALLOC_FAILURE);
    }

    if (!compression) { /* business as usual */
      if (sendAll(client->conn, response, &(node->spaceUsed)) < 0) {
        #ifdef DEBUG
          printf("proxy.c: Error forwarding response to client!\n");
        #endif
      }

      #ifdef DEBUG
        printf("proxy.c: %ld bytes sent to client.\n%s\n", node->spaceUsed, (char*)response);
      #endif
    } else { /* need to buffer the image and send it out all at once */
      imageBuffer = realloc(imageBuffer, sizeof(char) * (bytes + node->spaceUsed));
      if (!imageBuffer) { /* ....... */
        #ifdef DEBUG
          printf("proxy.c: Fatal memory error; cannot buffer image!\n");
        #endif
        compression = 0;
      }
      memcpy((char*)imageBuffer + bytes, response, node->spaceUsed);
    }

    /* a few menial tasks */
    bytes += node->spaceUsed;
    free(response);

    /* signal the server */
    pthread_cond_signal(&(node->condition)); 
  } while (node->serverState != COMPLETE);
  pthread_mutex_unlock(&(node->mutex));

  node->proxyState = IDLE;

  /* now check for image compression */
  if (compression) { /* we have a buffered image */
    xmlrpc_value* result;
    char* methodName = "compress";
    char serverURL[1000];

    /* set up the server name */
    memset(&serverURL, 0, sizeof(serverURL));
    snprintf(serverURL, sizeof(serverURL) - 1, "http://%s:%d/RPC2", distserver, distport);

    /* send the RPC request and retrieve the response */
    result = xmlrpc_client_call(environment, serverURL, methodName, "(6)", imageBuffer, bytes);
    rpcFault(environment);
    xmlrpc_decompose_value(environment, result, "(6)", &compImg, &compImgLen);
    rpcFault(environment);

    /* send the compressed image to the client! */
    if (sendAll(client->conn, compImg, &compImgLen) < 0) {
      #ifdef DEBUG
        printf("proxy.c: Error forwarding compressed image to client!\n");
      #endif
    }

    #ifdef DEBUG
      printf("proxy.c: %ld bytes of compressed image sent to client.\n", compImgLen);
    #endif
    
    free(imageBuffer);
    free(compImg);
    xmlrpc_DECREF(result);
  }

  /* success...holy shit */
  return 0;
}

/* This function simply determines whether the -c flag, which indicates
 * that the proxy server will compress any JPG images it receives,
 * has been used.
 *
 * @param numargs The number of command line arguments.
 * @param arguments The actual arguments.
 * @return 1 if the proxy is set for compression, 0 otherwise.
 */
static int isCompressed(int numargs, char** arguments) {
  int i;
  for (i = 0; i < numargs; i++) {
    if (strcmp(arguments[i], "-c") == 0) {
      return 1;
    }
  }
  return 0;
}

/* Prints error messages pertaining to the XML-RPC environment. 
 * This function indicates whether an error actually occurred, allowing
 * the programmer to decide how to handle this.
 *
 * @param environment The XML-RPC environment.
 * @return 1 if an error occurred, 0 otherwise.
 */
static int rpcFault(xmlrpc_env* const environment) {
  if (environment->fault_occurred) {
    printf("XML-RPC error: %s (%d)\n", environment->fault_string, environment->fault_code);
    return 1;
  }

  /* we're clean */
  return 0;
}
