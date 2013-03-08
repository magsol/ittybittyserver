#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/stat.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <dirent.h>

#include "headers/server.h"

/* function headers */

static void* handleClient(void* args);
static void checkAndSend(void* conn, int shared);
static void catchInterrupt(int signum);
static void initializeGlobals(void);
static void cleanUpGlobals(void);

/* global variables */

pthread_t* workers;		/* pool of worker threads */
pthread_attr_t scope;		/* set system scope of thread scheduling */
conlist* list;			/* the list of active clients' connections */
pthread_mutex_t mConList;	/* protects connection list */
pthread_cond_t free_conn;	/* a pending connection waits to be serviced */
int numThreads;			/* size of thread pool */
int serverSock;			/* local socket identifier */
int LOOP;			/* indicates if the main loop continues */
int OPTIMIZED;			/* is this server optimized? */
memnode* shList;		/* shared memory list */
metanode* shMeta;		/* shared metadata */

/* LET'S GET TO WORK */

int main(int argc, char** argv) {

  int clientSock;		/* client socket identifier */
  struct sockaddr_in localaddr;	/* local address struct */
  struct sockaddr_in clientaddr;	/* client address struct */
  struct sigaction sa;		/* responsible for trapping SIGINT */ 
  int i;

  if (argc == 2 && strcmp(argv[1], "DELETE") == 0) {
    shMeta = getMetanode();
    if (!shMeta->numNodes) {
      printf("Accidental use.  Exiting...\n");
      exit(INCORRECT_ARGS);
    }
    shList = getMemList(shMeta->numNodes);
    destroyMemList(shList, shMeta->numNodes);
    destroyMetanode(shMeta);
    printf("Shared memory cleaned up.\n");
    exit(1);
  }

  if (argc < 3) {
    printArgs(SERVER);
    exit(INCORRECT_ARGS);
  }

  /* optimization? */
  OPTIMIZED = isOptimized(argc, argv);

  if (((argc == 5 && chdir(argv[3]) < 0) || /* specified */
       (argc == 4 && OPTIMIZED && chdir(".") < 0) || /* not specified */
       (argc == 4 && !OPTIMIZED && chdir(argv[3]) < 0)) || /* specified */
       (argc == 3 && chdir(".") < 0)) { /* not specified */

    printf("Unable to read from document directory \"%s\".  Exiting...\n", (argc == 5 ? argv[3] : (argc == 4 ? (OPTIMIZED ? "." : argv[3]) : ".")));
    exit(INCORRECT_ARGS);
  }

  /******************************************\
  |* Initializations and memory allocations *|
  \******************************************/

  /* loop! */
  LOOP = 1;

  /* determine the number of threads */
  numThreads = atoi(argv[2]);
  if (numThreads <= 0) { /* you're dumb */
    printf("Invalid thread argument \"%d\".  Exiting...\n", numThreads);
    exit(INCORRECT_ARGS);
  }

  /* do everything else */
  initializeGlobals();

  /* construct the server information */
  memset(&localaddr, 0, sizeof(localaddr));
  localaddr.sin_family 		= AF_INET;
  localaddr.sin_addr.s_addr	= htonl(INADDR_ANY);
  localaddr.sin_port		= htons(atoi(argv[1])); 
  
  /* bind the server socket */
  if (bind(serverSock, (struct sockaddr *) &localaddr, sizeof(localaddr)) < 0) {
    printf("Error binding port to local address.  Exiting...\n");
    exit(SOCKET_FAILURE);
  }

  /* listen... */
  if (listen(serverSock, MAXCONNECTIONS_SERVER) < 0) {
    printf("Error listening for incoming connections.  Exiting...\n");
    exit(SOCKET_FAILURE);
  }

  /* everything is set up, so establish the interrupt handler */
  sa.sa_handler = catchInterrupt;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGINT, &sa, NULL);

  /*************************************\
  |* here's where the magic happens... *|
  \*************************************/

  /* start the worker threads */
  for (i = 0; i < numThreads; i++) {
    pthread_create(&workers[i], &scope, handleClient, (void *)i);
  }

  /* make the socket NONBLOCKING */
  if (OPTIMIZED && fcntl(serverSock, F_SETFL, F_GETFL | O_NONBLOCK) < 0) {
    #ifdef DEBUG
      printf("server.c: Unable to set server socket as nonblocking!\n");
    #endif
  }

  while (LOOP) { /* loop until this variable changes by way of SIGINT */
    unsigned int clientLength = sizeof(clientaddr);

    #ifdef DEBUG
      if (!OPTIMIZED) { /* just so this doesn't repeat CONSTANTLY */
        printf("Waiting for an incoming connection...\n");
      }
    #endif

    /* ---=SHARED MEMORY=--- */
    if (OPTIMIZED) {
      if (shMeta->proxyFlag == ONLINE) { /* possibility for use! */
        /* look for a request from the proxy */
        memnode* node = findState(shList, shMeta->numNodes, WAITING_INIT_SRVR);
        if (node && node->serverState == IDLE) { /* GOT A REQUEST */
          node->serverState = BUSY;
          pthread_mutex_lock(&mConList);
          addTail(0, SHARED, list);
          pthread_cond_broadcast(&free_conn);
          pthread_mutex_unlock(&mConList);
        }  
      }
    }
    /* ---=END SHARED MEMORY=--- */

    if ((clientSock = accept(serverSock, (struct sockaddr *) &clientaddr, &clientLength)) < 0) {
      if (!LOOP) { /* the loop was broken, so the socket is closed! */
        break;
      } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
        continue; /* we're just fine */
      }

      /* if we get here, then there's an actual problem */
      printf("Error accepting incoming connection.  Exiting...\n");
      exit(SOCKET_FAILURE);
    } else {

      #ifdef DEBUG
        char* client_Addr = inet_ntoa(clientaddr.sin_addr);
        printf("Server: Connection from %s\n", client_Addr);
      #endif
    
      pthread_mutex_lock(&mConList);
      addTail(clientSock, PROCESS, list);
      pthread_cond_broadcast(&free_conn);
      pthread_mutex_unlock(&mConList);
    }
    /* keep it goin' */
  }

  /*********************\
  |* that's all folks! *|
  \*********************/

  cleanUpGlobals();

  printf("Finished! :)\n");

  return SUCCESS;
}

/*
 * This function is called by any worker threads.  It finds and locates the 
 * requested file in the root (and does security checking to ensure the ".."
 * is not honored), constructs a valid HTTP response, and sends it on its
 * merry way to the client.
 *
 * args is the unique thread ID corresponding to the position in the "available" array
 */
static void* handleClient(void* args) {
  connection* c;
  int ID = (int)args;
  
  #ifdef DEBUG
    printf("Thread %d executing\n", ID);
  #endif 
 
  while (1) { /* loop indefinitely, or until this thread quits */
 
    /* sit and wait until there's a connection available, then nab it */
    pthread_mutex_lock(&mConList);
    while (isEmpty(list)) {
      pthread_cond_wait(&free_conn, &mConList);
    }
    c = removeHead(list);
    pthread_mutex_unlock(&mConList);

    #ifdef DEBUG
      if (c->action == PROCESS) {
        printf("Thread %d given a client.\n", ID);
      } else if (c->action == SHARED) {
        printf("Thread %d processing a shared memory request.\n", ID);
      } else { /* terminate! */
        printf("Thread %d beginning termination!\n", ID);
      }
    #endif

    if (c->action == TERMINATE) { /* time to exit! */
      free(c);

      #ifdef DEBUG
        printf("Thread %d terminated.\n", ID);
      #endif

      pthread_exit(0);
    }

    /* now process this connection! */
    if (c->action == SHARED) {
      memnode* node = findState(shList, shMeta->numNodes, WAITING_INIT_SRVR);
      node->serverState = WAITING_CONT_PRXY;
 
      /* signal the waiting proxy that we have found the connection */
      /* pthread_cond_signal(&(node->condition)); */
      /* lock the mutex */
      pthread_mutex_lock(&(node->mutex));

      /* process the connection */ 
      checkAndSend(node, 1);
      free(c);

      #ifdef DEBUG
        printf("server.c: Shared connection successfully processed and closed!\n");
      #endif
      node->serverState = IDLE;
    } else {
      checkAndSend(c, 0);
      close(c->conn);
      free(c);
    }
  } /* end while loop */
  /* no need for a return statement, since execution will never get here */
}

/*
 * The connection type has been abstracted out via a "void*" cast, and
 * only when a message is send by way of sendResponse will the abstraction
 * be cast aside.
 *
 * This function takes care of processing the incoming connection, checking
 * for any errors, and generating a response.
 *
 * NOTE: This function makes the HORRIBLE TERRIBLE AWFUL assumption that
 *       no request header will EVER exceed 20,0000 bytes.  This size
 *       can obviously be changed through the code, but this decision
 *       was made for simplicity's sake, in spite of the obvious
 *       effects on scalability.
 *       
 *       Bottom line, sue me.
 *
 * @param conn The abstracted connection data type.
 * @param shared A flag indicating whether this conection's communication
 *               is to take place through shared memory or sockets.
 */
static void checkAndSend(void* conn, int shared) {
  struct stat sb;
  char line[20000], method[10000], path[10000], protocol[10000], location[10000], idx[10000];
  void* contents;
  int fileLen;
  char* file;

  memset(&line, 0, sizeof(line));

  /* this is the ONLY TIME this function will specifically check shared */
  if (!shared) {
    connection* c = (connection*)conn;
    if (recv(c->conn, line, sizeof(line) - 1, 0) < 0) {
      sendError(400, "Bad Request", (char*)0, "No request found.\n", conn, shared);
      return; /* nothing else to do here */
    }
  } else { /* shared memory */
    void* request;
    long int bytes = 0;
    memnode* node = (memnode*)conn;

    #ifdef DEBUG
      printf("server.c: Beginning shared memory read of request.\n");
    #endif

    /* now signal the proxy */
    pthread_cond_signal(&(node->condition));

    /* loop on receive until nothing is left */
    do {
      /* wait if the proxy hasn't signaled yet */
      if (node->proxyState != COMPLETE) {
      #ifdef DEBUG
        printf("Waiting...\n");
      #endif
        pthread_cond_wait(&(node->condition), &(node->mutex));
      }
      #ifdef DEBUG
        printf("Continuing...\n");
      #endif

      /* once the proxy signals, read the data */
      if ((request = receiveShared(node)) == NULL) {
        printf("FATAL SERVER ERROR: UNABLE TO READ SHARED MEMORY!\n");
        exit(MEMALLOC_FAILURE);
      }
      memcpy((line + bytes), request, node->spaceUsed);

      /* a few menial tasks */
      bytes += node->spaceUsed;
      free(request);

      /* signal the proxy that we're ready for another chunk */
      if (node->proxyState != COMPLETE) {
        pthread_cond_signal(&(node->condition));
      }
    } while (node->proxyState != COMPLETE);

    /* mutex was locked prior to calling checkAndSend - unlock it */
    node->serverState = BUSY;
  }

  #ifdef DEBUG
    printf("server.c: Request received, processing.\n");
  #endif

  /* from here on, abstraction will be used */
  /* continue checks against input */

  /* CHECK FOR SUCCESSFUL PARSING */
  if (sscanf(line, "%[^ ] %[^ ] %[^ ]", method, path, protocol) != 3) {
    sendError(400, "Bad Request", (char*)0, "Can't parse request.\n", conn, shared);
    return;
  }

  /* CHECK FOR CORRECT HTML METHOD */
  if (strcasecmp(method, "get") != 0) {
    sendError(501, "Not Implemented", (char*)0, "That method is not implemented.\n", conn, shared);
    return;
  }

  /* CHECK FOR CORRECT PATHNAME SYNTAX */
  if (path[0] != '/') {
    sendError(400, "Bad Request", (char*)0, "Bad filename.\n", conn, shared);
    return;
  }

  /* SET UP FILE ACCESS */
  file = &(path[1]); /* first character, aside from forward slash */
  strDecode(file, file); /* get rid of HTML encodings in file */
  if (file[0] == '\0') { /* this means only the root was requested */ 
    file = "./";
  }
  fileLen = strlen(file);

  /* CHECK FOR ATTEMPS AT FORBIDDEN FILE READS */
  if (file[0] == '/' || strcmp(file, "..") == 0 || 
      strncmp(file, "../", 3) == 0 || strstr(file, "/../") != (char*)0 ||
      strcmp(&(file[fileLen - 3]), "/..") == 0) { /* evil */

    sendError(400, "Bad Request", (char*)0, "Illegal filename.\n", conn, shared);
    return;
  }

  /* CHECK FOR LEGAL REQUESTED FILE */
  if (stat(file, &sb) < 0) {
    sendError(404, "Not Found", (char*)0, "File not found.\n", conn, shared);
    return;
  }

  /* DETERMINE IF REQUEST IS FOR A FILE OR DIRECTORY */
  if (S_ISDIR(sb.st_mode)) { /* request is for a directory */
    if (file[fileLen - 1] != '/') { /* append trailing slash to URL */
      snprintf(location, sizeof(location) - 1, "Location: %s/%s", path, EOL);
      sendError(302, "Found", location, "Directories must end with a slash.\n", conn, shared);
      return;
    }

    snprintf(idx, sizeof(idx) - 1, "%sindex.html", file);
    if (stat(idx, &sb) >= 0) { /* this file exists */
      int filedesc;
      file = idx;
      filedesc = open(file, O_RDONLY);
      if (!(contents = fileContents(filedesc, sb.st_size))) {
        sendError(403, "Forbidden", (char*)0, "File is protected.\n", conn, shared);
        return;
      }
      sendResponse(200, "OK", (char*)0, contentType(file), sb.st_size, contents, conn, shared);
      if (munmap(contents, sb.st_size) < 0) {
        printf("server.c: Cannot unmap file contents!\n");
      }
      close(filedesc);
    } else { /* print out directory contents */
      struct dirent **dl;
      long int size;
      char buf[10000];
      int k, n = scandir(file, &dl, NULL, alphasort);
      contents = NULL;
      if (n < 0) { /* didn't find any files */
        sendError(500, "Internal Server Error", (char*)0, "The server encountered an error.\n", conn, shared);
        return;
      }
      snprintf(buf, sizeof(buf) - 1, "<html><head><title>Index of %s</title></head>\n<body bgcolor=\"#99CC99\"><h3>Index of %s</h3>\n<pre>\n", file, file);
      contents = appendStr(contents, buf);
      size = strlen(buf);
      for (k = 0; k < n; k++) {
        int before, after;
        before = strlen((char*)contents);
        contents = appendStr(contents, dl[k]->d_name);
        after = strlen((char*)contents);
        size += (after - before);
        free(dl[k]);
      }
      memset(&buf, 0, sizeof(buf));
      snprintf(buf, sizeof(buf) - 1, "</pre>\n<hr /><address><a href=\"%s\">%s</a></address>\n</body></html>\n", SERVER_URL, SERVER_NAME);
      contents = appendStr(contents, buf);
      size += strlen(buf);

      #ifdef DEBUG
        printf("SIZE: %ld\nLENGTH: %d\nSTRING: %s\n", size, strlen((char*)contents), (char*)contents);
      #endif

      sendResponse(200, "OK", (char*)0, "text/html", size, contents, conn, shared);
      free(contents);
      free(dl);
    }
  } else { /* request is for a flat file */
    int filedesc = open(file, O_RDONLY);
    contents = fileContents(filedesc, sb.st_size);
    if (!contents) { /* assuming bad file permissions */
      sendError(403, "Forbidden", (char*)0, "File is protected.\n", conn, shared);
      return;
    }

    /* send everything on its merry way */
    sendResponse(200, "OK", (char*)0, contentType(file), sb.st_size, contents, conn, shared);
    if (munmap(contents, sb.st_size) < 0) {
      printf("server.c: Cannot unmap file contents!\n");
    }
    close(filedesc);
  }

  /* everything was successful! */
  #ifdef DEBUG
    printf("server.c: checkAndSend successful!\n");
  #endif
}

/*
 * Responsible for catching a CTRL+C action and cleaning up gracefully.
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

  /* cannot protect the whole operation with a loop,
   * otherwise we risk deadlock.  This will take longer,
   * but the list will be protected only when we add
   * a new termination node
   */
  LOOP = 0; /* this will kill the loop in main() */
  for (i = 0; i < numThreads; i++) {
    /* add a termination node for every active thread */
    pthread_mutex_lock(&mConList);
    addTail(0, TERMINATE, list);
    pthread_cond_broadcast(&free_conn);
    pthread_mutex_unlock(&mConList);
  }

  #ifdef DEBUG
    printf("All termination tokens added.  Waiting for exit...\n");
  #endif

  /* let execution continue normally */
}

/* This helper function simply shortens the amount of code in main()
 * by performing global variable initializations here instead.
 */
static void initializeGlobals(void) {
  
  /* initialize the mutex */
  if (pthread_mutex_init(&mConList, NULL) != 0) { 
    printf("Error initializing mutex.  Exiting...\n");
    exit(MUTEX_FAILURE);
  }

  /* initialize condition variable */
  if (pthread_cond_init(&free_conn, NULL) != 0) {
    printf("Error initializing condition variable.  Exiting...\n");
    exit(MUTEX_FAILURE);
  }

  /* allocate the worker pool */
  if (!(workers = malloc(sizeof(pthread_t) * numThreads))) {
    printf("Error allocating memory for thread pool.  Exiting...\n");
    exit(MEMALLOC_FAILURE);
  }

  /* set up the thread attribute */
  pthread_attr_init(&scope);
  pthread_attr_setscope(&scope, PTHREAD_SCOPE_SYSTEM);
  pthread_attr_setdetachstate(&scope, PTHREAD_CREATE_JOINABLE);

  /* set up connections list */
  list = newConList();
  if (!list) {
    printf("Error allocating memory for connections list.  Exiting...\n");
    exit(MEMALLOC_FAILURE);
  }

  /* set up the server socket */
  if ((serverSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    printf("Error opening socket for listening.  Exiting...\n");
    exit(SOCKET_FAILURE);
  }

  /* set up the shared memory */
  if (OPTIMIZED) {
    int nodes = numThreads;

    shList = getMemList(nodes);
    if (!shList) { /* possible too many segments for the system to handle */
      nodes = NUMSEGMENTS;
      shList = getMemList(nodes); /* try the default number */
      if (!shList) { /* well you're pretty much screwed */
        printf("ERROR: System is unable to allocate enough shared memory given your current settings.  Try specifying fewer worker threads, or possibly your system cannot handle the shared memory requirements, in which case you cannot use the \"-o\" runtime flag.\n\nExiting...\n");
        exit(MEMALLOC_FAILURE);
      }
    }

    /* now allocate the metanode */
    shMeta = getMetanode();
  
    if (!shMeta) {
      destroyMemList(shList, nodes);
      printf("Unable to allocate space for metanode.  Exiting...\n");
      exit(MEMALLOC_FAILURE);
    }

    /* set the server to online */
    shMeta->serverFlag = ONLINE;
    shMeta->numNodes = nodes;

    /* DEBUG output for shared operations already taken care of in memList.c */
  }

  /* that's all we can really do from outside the main() */
}

/* This helper function undoes most of the tasks performed by
 * initializeGlobals(), freeing resources and generally cleaning
 * up everything the server utilized.
 */
static void cleanUpGlobals(void) {
  int i;
  void* status;
  
  /* free up threads first, in case the mutex is locked */
  for (i = 0; i < numThreads; i++) {
    int retval = pthread_join(workers[i], &status);
    if (retval) { /* doh */
      printf("Error joining thread %d. Code: %d\n", i, retval);
    }

    #ifdef DEBUG
      printf("Thread %d joined! Status: %ld\n", i, (long)status);
    #endif
  }

  /* destroy mutex, condition variable, and attribute struct */
  if (pthread_mutex_destroy(&mConList) != 0) {
    printf("Error destroying connection mutex!\n");
  }
  #ifdef DEBUG
    else {
    printf("Connection mutex successfully destroyed.\n");
  }
  #endif
 
  if (pthread_cond_destroy(&free_conn) != 0) {
    printf("Error destroying connection condition variable!\n");
  }
  #ifdef DEBUG
    else {
    printf("Connection condition variable successfully destroyed.\n");
  }
  #endif
 
  if (pthread_attr_destroy(&scope) != 0) {
    printf("Error destroying thread attributes!\n");
  }
  #ifdef DEBUG
    else {
    printf("Thread attributes successfully destroyed.\n");
  }
  #endif

  /* destroy the connections list */
  /* NOTE: No need to systematically close each socket connection;
   * as each thread exited, they should have closed their own sockets
   */
  destroyAll(list);
  free(list);

  /* destroy the list of workers */
  free(workers);

  /* close the socket! */
  if (close(serverSock) < 0) {
    printf("Error closing server socket!\n");
  }

  /* shared memory? */
  if (OPTIMIZED) {
    if (destroyMemList(shList, shMeta->numNodes) < 0) {
      printf("Failure destroying shared memory list!\n");
    }

    if (destroyMetanode(shMeta) < 0) {
      printf("Failure destroying metanode!\n");
    }

    /* DEBUG output for these operations is already taken care of
     * within memList.c 
     */
  }
}
