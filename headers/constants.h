#ifndef _CONSTANTS_
#define _CONSTANTS_

/* program constants */

#define SERVER_NAME "Squinn!"
#define SERVER_URL "http://www.magsolweb.net"
#define VERSION "v1.5"
#define PROTOCOL "HTTP/1.0"
#define EOL "\r\n"
#define MAXCONNECTIONS_SERVER 10
#define MAXCONNECTIONS_PROXY 10
#define NUMACCESSES 10

/* shared memory constants */

#define SEGMENTSIZE 10000
#define NUMSEGMENTS 10
#define SEGMENTNAME "/segmentlist3"
#define METANODENAME "/metanode"

/* program identifiers */

#define SERVER 1
#define CLIENT 2
#define PROXY 3


#endif /* _CONSTANTS_ */
