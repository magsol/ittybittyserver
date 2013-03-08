#include <stdlib.h>
#include <stdio.h>

#include "conList.h"

/* the connection struct */ 
/*
typedef struct connection {
  int conn;
  struct connection* next;
} connection;
*/
/* REMEMBER THE THREE CASES *\
 *
 * 1) Entire list is null
 * 2) Only 1 item in list
 * 3) Lots of items in list
 */

conlist* newConList(void) {
  conlist* toReturn = malloc(sizeof(conlist));
  if (!toReturn) {
    return NULL;
  }

  toReturn->head = NULL;
  toReturn->tail = NULL;
  toReturn->numNodes = 0;

  return toReturn;
}

/*
 * A nice utility function for building and returning new connection nodes.
 * 
 * @param conID The socket connection identifier.
 * @param a The instruction enum for this node.
 * @return A new connection node.
 */
connection* newConnection(int conID, instruction a) {
  connection* toReturn = malloc(sizeof(connection));
  if (!toReturn) {
    return NULL;
  }

  toReturn->conn   = conID;
  toReturn->action = a;
  toReturn->next   = NULL;
  return toReturn;
}

/*
 * This function returns 0 if the list has something in it, nonzero
 * if the list is EMPTY.
 */
int isEmpty(conlist* list) {
  if (!list->head) {
    return 1;
  } else {
    return 0;
  }
}

/*
 * Using the connection ID integer, this builds a new connection nodes,
 * tacks it to the front of the list, and returns a pointer to it.
 *
 * @param conID The socket connection identifier.
 * @param a The enum indicating the action this node will instruct.
 * @param list A pointer to the connection list.
 */
void addHead(int conID, instruction a, conlist* list) {
  connection* newHead = newConnection(conID, a);
  if (!newHead) {
    return;
  }

  newHead->next = list->head;
  list->head = newHead;
  if (!list->tail) {
    list->tail = list->head;
  }
  list->numNodes++;
}

/*
 * Same functionality as addHead(), except this adds to the rear of
 * the list.
 *
 * @param conID The socket connection identifier.
 * @param a The enum indicating the action this node will take.
 * @param list A pointer to the connections list.
 */
void addTail(int conID, instruction a, conlist* list) {
  connection* newTail = newConnection(conID, a);
  if (!newTail) {
    return;
  }

  if (!list->head) { /* empty list */
    list->head = newTail;
  } else { /* at least one element */
    list->tail->next = newTail;
  }
  list->tail = newTail;
  list->numNodes++;
}

/*
 * This function chops off the head of the list and returns it as a 
 * single node.
 */
connection* removeHead(conlist* list) {
  connection* toReturn = list->head;
  if (!toReturn) { /* empty list */
    return NULL;
  } 

  if (list->head == list->tail) { /* one element */
    list->head = NULL;
    list->tail = NULL;
  } else { /* n-element list */
    list->head = list->head->next;
  }

  return toReturn;
}

/*
 * Given a connection ID, this searches the list of active connections for the
 * node with the matching ID and returns a pointer to this node.
 */
connection* findCon(int conID, conlist* list) {
  connection* node = list->head;
  while (node) {
    if (node->conn == conID) {
      return node;
    }
    node = node->next;
  }

  return NULL;
}

/*
 * This utility function finds and frees the node with the given connection ID, 
 * returning the head of the list (which may have changed).  Returns the original
 * list if the correct connection is not found.
 */
void destroyCon(int conID, conlist* list) {
  connection* node = list->head, *prev = NULL;
  while (node) {
    if (node->conn == conID) {
      if (prev) { /* the head of the list is not the node we're deleting */
        prev->next = node->next;
      }
 
      if (node == list->tail) { /* reassign tail, if necessary */
        list->tail = prev;
      }
      free(node);
      list->numNodes--;
      return;
    }
    
    prev = node;
    node = node->next;
  }
}

/*
 * Kills the entire list.
 * NOTE: Caller must free the argument explicitly!
 */
void destroyAll(conlist* list) {
  connection* node = list->head, *prev = NULL;

  while (node) {
    prev = node;
    node = node->next;
    free(prev);
  }
}

/*
 * For debugging purposes.
 */
void printList(conlist* list) {
  connection* node = list->head;
  while (node) {
    printf("%d\n", node->conn);
    node = node->next;
  }
}
/*
int main(int argc, char** argv) {

  conlist* theList = newConList();
  int i, conn;
  if (argc < 3) {
    printf("Usage: > ./list <numNodes> <random # limit>\n");
    exit(1);
  }

  for (i = 0; i < atoi(argv[1]); i++) {
    conn = rand();
    conn %= atoi(argv[2]);

    addHead(conn, theList);
    addTail(conn, theList);
  }

  free(removeHead(theList));

  printList(theList);

  destroyAll(theList);

  free(theList);
  return 0;
}*/
