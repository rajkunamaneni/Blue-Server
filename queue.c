#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define BLOCK 2048

//
// Bounded Queue used for storing the request for
// processing for thread pool.
//

typedef struct Queue {
  int *data;
  int head;
  int tail;
  int size;
} Queue;

// Constructors-Destructors ---------------------------------------------------

//
// Construct Queue Structure
//
Queue *queue_create(void) {
  Queue *q = (Queue *)malloc(sizeof(Queue));
  if (q != NULL) {
    q->head = q->size = q->tail = 0;
    q->data = (int *)malloc(BLOCK * sizeof(int));
  }
  return q;
}

//
// Free Queue
//
void freeQueue(Queue **q) {
  if (*q) {
    free((*q)->data);
    free(*q);
    *q = NULL;
  }
  return;
}

// Functions ------------------------------------------------------------------

//
// Return the size of the Queue
//
int queue_size(Queue *q) { return q->size; }

//
// Check if Queue is full
//
bool queue_full(Queue *q) { return (q->size == BLOCK); }

//
// Check if Queue is empty
//
bool queue_empty(Queue *q) { return (q->size == 0); }

//
// Enqueue to Queue
//
bool enqueue(Queue *q, int x) {
  if (queue_full(q)) {
    return false;
  }
  q->data[q->tail] = x;

  q->tail = (q->tail + 1);
  q->size += 1;
  return true;
}

//
// Dequeue to Queue
//
bool dequeue(Queue *q, int *x) {
  if (queue_empty(q)) {
    return false;
  }
  *x = q->data[q->head];

  q->head = (q->head + 1);
  q->size -= 1;
  return true;
}

//
// Print Queue.
// Created for Debugging
//
void print_queue(Queue *q) {
  for (int i = 0; i < q->size; i += 1) {
    printf("%d", q->data[(q->head + i) % BLOCK]);
    if (i + 1 != q->size) {
      printf(", ");
    }
  }
  printf("\n");
  return;
}
