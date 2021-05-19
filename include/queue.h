#ifndef QUEUE_H
#define QUEUE_H

#include <stdint.h>
#include <stdbool.h>

typedef struct queue_item {
  void *unused_ptr; // because empty structures are undefinde behavior
} queue_item_t;

typedef struct queue queue_t;

// CONSTUCT
queue_t* queue_create(uint32_t min_size);
void queue_free(queue_t *q);

// ATD
void queue_push(queue_t *q, queue_item_t *it);
queue_item_t *queue_pop(queue_t *q);
bool queue_is_empty(const queue_t *q);

#endif // QUEUE_H
