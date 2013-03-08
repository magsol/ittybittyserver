#ifndef _RECVALL_
#define _RECVALL_

void* recvAll(int socket, long int* headerLength, long int* bodyLength);
long int recvAll_NoData(int socket, long int* bodyLength);
void* recvHeader(int socket, long int* headerLength, long int* bodyLength);
void* recvHeader_Mem(void* mem, long int* headerLength, 
                     long int* bodyLength);
void* recvBody_Mem(void* mem, long int bodyLength, long int* bytesCopied,
                   int isHeader);

#include "recvAll.c"

#endif /* _RECVALL_ */
