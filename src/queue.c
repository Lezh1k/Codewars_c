#include "queue.h"

#include <stddef.h>
#include <stdlib.h>
#include "commons.h"

struct queue {
  uint32_t size, head, tail;
  queue_item_t **data;
};
//////////////////////////////////////////////////////////////

queue_t *
queue_create(uint32_t min_size) {
  min_size = nearest_power_of_2_u32(min_size);
  queue_t *q = malloc(sizeof(queue_t));
  if (q == NULL) {
    return NULL;
  }

  q->size = min_size;
  q->tail = 0;
  q->head = min_size;
  q->data = malloc(q->size * sizeof(queue_item_t*));

  if (q->data == NULL) {
    free(q);
    return NULL;
  }

  return q;
}
//////////////////////////////////////////////////////////////

void
queue_free(queue_t *q) {
  if (q) free(q->data);
  free(q);
}
//////////////////////////////////////////////////////////////

void
queue_push(queue_t *q,
           queue_item_t *it) {
  q->data[q->tail++] = it;
  if (q->tail != q->head) {
    q->tail %= q->size;
    return; // we don't need do anything else
  }

  // handle full queue
  queue_t *tmp = queue_create(q->size*2);
  for (uint32_t i = 0; i < q->size; ++i) {
    queue_push(tmp, queue_pop(q));
  }

  q->size = tmp->size;
  q->tail = tmp->tail;
  q->head = tmp->head;
  free(q->data);
  q->data = tmp->data;
  free(tmp);
}
//////////////////////////////////////////////////////////////

queue_item_t *
queue_pop(queue_t *q) {
  q->head %= q->size;
  return q->data[q->head++];
}
//////////////////////////////////////////////////////////////

bool
queue_is_empty(const queue_t *q) {
  return q->head % q->size == q->tail;
}
//////////////////////////////////////////////////////////////

