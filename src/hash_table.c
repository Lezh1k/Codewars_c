#include "hash_table.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

ht_item_t *ht_item_new(void) {
  ht_item_t *res = malloc(sizeof(ht_item_t));
  if (!res) {
    return res;
  }
  res->key = NULL;
  res->val = NULL;
  return res;
}

ht_item_t *ht_item_new_key_val(const char *key, const char *val) {
  ht_item_t *res = malloc(sizeof(ht_item_t));
  if (!res) {
    return res;
  }
  res->key = strdup(key);
  res->val = strdup(val);
  return res;
}

void ht_item_free(ht_item_t *it) { free(it); }
//////////////////////////////////////////////////////////////

ht_t *ht_new(void) {
  return NULL;
}

void ht_free(ht_t *ht) {
}
//////////////////////////////////////////////////////////////
