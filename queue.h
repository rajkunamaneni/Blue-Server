#pragma once

#include <stdbool.h>

typedef struct Queue Queue;

Queue *queue_create(void);

void freeQueue(Queue **q);

int queue_size(Queue *q);

bool queue_full(Queue *q);

bool queue_empty(Queue *q);

bool enqueue(Queue *q, int x);

bool dequeue(Queue *q, int *x);

void print_queue(Queue *q);
