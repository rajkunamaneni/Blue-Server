#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "list.h"

//
// Linked List for storing header-fields.
//

typedef struct NodeObj *Node;

typedef struct NodeObj {
    void *data;
    Node next;
    Node prev;
} NodeObj;

typedef struct ListObj {
    Node front;
    Node back;
    Node cursor;
    int index;
    int length;
} ListObj;

// Constructors-Destructors ---------------------------------------------------

//
// Constructors for new Node in Linked List
//
Node newNode(void *data) {
    Node N = malloc(sizeof(NodeObj));
    N->data = data;
    N->next = NULL;
    N->prev = NULL;
    return (N);
}

//
// Free Nodes
//
void freeNode(Node *pN) {
    if (pN != NULL && *pN != NULL) {
        free(*pN);
        *pN = NULL;
    }
}

//
// Creates and returns a Linked List L
//
List newList(void) {
    List L = malloc(sizeof(ListObj));
    L->front = L->back = L->cursor = NULL;
    L->index = -1;
    L->length = 0;
    return (L);
}

//
// Frees all heap memory associated with *pL
//
void freeList(List *pL) {
    if (pL != NULL && *pL != NULL) {
        while ((*pL)->length != 0) {
            deleteFront(*pL);
        }
    }
    free(*pL);
    *pL = NULL;
}

// Functions ------------------------------------------------------------------

//
// Check if Linked List is empty in L
//
bool empty(List L) {
    if (L == NULL) {
        return false;
    }
    return (L->length == 0);
}

//
// Return the current placement of cursor in L
//
int cursor(List L) {
    if (L == NULL) {
        exit(EXIT_FAILURE);
    }
    return (L->index);
}

//
// Return the length of the L
//
int length(List L) {
    if (L == NULL) {
        exit(EXIT_FAILURE);
    }
    return (L->length);
}

//
// Returns front element of L
//
void *front(List L) {
    if (L == NULL) {
        return false;
    }
    if (length(L) == 0) {
        return false;
    }
    return (L->front->data);
}

//
// Returns the back element of L
//
void *back(List L) {
    if (L == NULL) {
        return false;
    }
    if (empty(L)) {
        return false;
    }
    return (L->back->data);
}

//
// Returns cursor element of L
//
void *get(List L) {
    if (L == NULL) {
        exit(EXIT_FAILURE);
    }
    if (length(L) == 0) {
        exit(EXIT_FAILURE);
    }
    if (cursor(L) < 0) {
        exit(EXIT_FAILURE);
    }
    return (L->cursor->data);
}

//
// If L is non-empty, sets cursor under the front element.
//
void moveFront(List L) {

    if (L == NULL) {
        exit(EXIT_FAILURE);
    }
    if (!empty(L)) {
        L->cursor = L->front;
        L->index = 0;
    }
    return;
}

//
// If L is non-empty, sets cursor under the back element.
//
//
void moveBack(List L) {
    if (L == NULL) {
        exit(EXIT_FAILURE);
    }
    if (!empty(L)) {
        L->cursor = L->back;
        L->index = L->length - 1;
    }
    return;
}

//
// If cursor is defined and not at front, move cursor one
// step toward the front of L; if cursor is defined and at
// front, cursor becomes undefined; if cursor is undefined
// do nothing
//
void movePrev(List L) {
    if (L == NULL) {
        exit(EXIT_FAILURE);
    }
    if (L->cursor == L->front) {
        L->cursor = NULL;
        L->index = -1;
        return;
    }
    if (L->cursor != L->front) {
        L->cursor = L->cursor->prev;
        L->index--;
    }
    return;
}

//
// If cursor is defined and not at back, move cursor one
// step toward the back of L; if cursor is defined and at
// back, cursor becomes undefined; if cursor is undefined
// do nothing
//
void moveNext(List L) {

    if (L == NULL) {
        exit(EXIT_FAILURE);
    }
    if (L->cursor == L->back) {
        L->cursor = NULL;
        L->index = -1;
        return;
    }
    if (L->cursor != L->back) {
        L->cursor = L->cursor->next;
        L->index++;
    }
    return;
}

//
// Insert new element into L.
//
void append(List L, void *x) {

    if (L == NULL) {
        exit(EXIT_FAILURE);
    }
    Node n = newNode(x);
    if (!empty(L)) {
        L->back->next = n;
        n->prev = L->back;
        L->back = n;
        L->length++;
        return;
    }
    L->front = n;
    L->back = n;
    L->index++;
    L->length++;
    return;
}

//
// Insert new element into L.
//
void prepend(List L, void *x) {
    if (L == NULL) {
        exit(EXIT_FAILURE);
    }
    Node n = newNode(x);
    if (!empty(L)) {
        L->front->prev = n;
        n->next = L->front;
        L->front = n;
        L->length++;
        if (L->index != -1 || L->cursor != NULL) {
            L->index++;
        }
        return;
    }
    L->front = n;
    L->back = n;
    L->index++;
    L->length++;
    return;
}

//
// Delete the front element and helper
// function for freeList.
//
void deleteFront(List L) {
    if (L == NULL) {
        exit(EXIT_FAILURE);
    }
    if (empty(L)) {
        exit(EXIT_FAILURE);
    }
    Node var = L->front;
    if (L->cursor == L->front) {
        L->index = -1;
        L->cursor = NULL;
    }
    if (L->length == 1) {
        L->front = L->back = NULL;
        freeNode(&var);
        L->length--;
        return;
    }
    L->front->next->prev = NULL;
    L->front = L->front->next;
    freeNode(&var);
    L->length--;
    if (L->cursor != NULL && L->index >= 0) {
        L->index--;
    }
    return;
}

//
// Delete the back element.
//
bool deleteBack(List L) {

    if (L == NULL) {
        return false;
    }
    if (empty(L)) {
        return false;
    }
    if (L->cursor == L->back) {
        L->index = -1;
        L->cursor = NULL;
    }
    Node var = L->back;
    if (L->length == 1) {
        L->front = L->back = NULL;
        freeNode(&var);
        L->length--;
        return true;
    }
    L->back->prev->next = NULL;
    L->back = L->back->prev;
    freeNode(&var);
    L->length--;
    return true;
}
