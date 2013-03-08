#ifndef CONLIST_H
#define CONLIST_H

#include <stdlib.h>
#include <stdio.h>

/* This file stores all the information regarding connection
 * lists.
 *
 * The type "instruction" stores different possible actions for
 * the specific node in which it resides to take.  As more possible
 * actions are added, they will be documented here.
 *
 * PROCESS: Node contains a valid socket identifier to be processed.
 * SHARED: Thread should read shared memory for further instructions. 
 * TERMINATE: Thread receiving this node should terminate.
 *
 * The type "connection" is a single node storing a socket identifier,
 * a subsequent action to take, and a pointer to the next node in the
 * list of nodes.
 *
 * The type "conlist" is a list of connection nodes, containing a pointer
 * to both the first and last nodes in the list.
 */

/* the instruction type */
typedef enum instruction {
  PROCESS,
  SHARED,
  TERMINATE
} instruction;

/* the connection node */
typedef struct connection {
  int conn; /* default connection */
  instruction action;
  struct connection* next;
} connection;

/* the list of connection nodes */
typedef struct conlist {
  connection* head;
  connection* tail;
  int numNodes;
} conlist;

/* functions */
conlist* newConList(void);
connection* newConnection(int conID, instruction a);
void addHead(int conID, instruction a, conlist* list);
void addTail(int conID, instruction a, conlist* list);
connection* removeHead(conlist* list);
connection* findCon(int conID, conlist* list);
void destroyCon(int conID, conlist* list);
void destroyAll(conlist* list);
void printList(conlist* list);

#include "conList.c"
#endif /* CONLIST_H */
