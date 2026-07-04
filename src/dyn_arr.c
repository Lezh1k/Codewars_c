#include "dyn_arr.h"
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

dyn_array_t *dynarr(size_t cap) {
  dyn_array_t *da = malloc(sizeof(dyn_array_t));
  da->size = 0;
  da->cap = cap;
  da->data = calloc(cap, sizeof(da_node_t *));
  return da;
}
//////////////////////////////////////////////////////////////

void da_free(dyn_array_t *da) {
  if (da->data != NULL) {
    free(da->data);
  }
  free(da);
}
//////////////////////////////////////////////////////////////

int da_push(dyn_array_t *da, da_node_t *node) {
  if (da->size >= da->cap) {
    da_node_t **tmp = realloc(da->data, da->cap * 2 * sizeof(da_node_t *));
    if (tmp == NULL) {
      return -errno;
    }
    da->data = tmp;
    da->cap *= 2;
  }
  da->data[da->size++] = node;
  return 0;
}
//////////////////////////////////////////////////////////////

da_node_t *da_pop(dyn_array_t *da) {
  if (da->size == 0) {
    return NULL;
  }
  return da->data[--da->size];
}
//////////////////////////////////////////////////////////////
