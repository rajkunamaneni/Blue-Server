#pragma once

#include <stdbool.h>

typedef struct ListObj *List;

List newList(void);

void freeList(List *pL);

bool empty(List L);

int cursor(List L);

int length(List L);

void *front(List L);

void *back(List L);

void *get(List L);

void moveFront(List L);

void moveBack(List L);

void movePrev(List L);

void moveNext(List L);

void prepend(List L, void *x);

void append(List L, void *x);

void deleteFront(List L);

bool deleteBack(List L);
