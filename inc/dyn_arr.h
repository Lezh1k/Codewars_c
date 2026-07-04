#ifndef DYN_ARR_H
#define DYN_ARR_H

#include <stddef.h>
#include <stdint.h>


typedef struct da_node {
  int8_t unused;
} da_node_t;

typedef struct dyn_array {
  da_node_t **data;
  size_t cap;
  size_t size;
} dyn_array_t;

dyn_array_t *dynarr(size_t cap);
void da_free(dyn_array_t *da);
int da_push(dyn_array_t *da, da_node_t *node);
da_node_t* da_pop(dyn_array_t *da);

#endif
