#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <stddef.h>
#include <stdint.h>

typedef struct ht_item {
  char *key;
  char *val;
} ht_item_t;

ht_item_t *ht_item_new(void);
ht_item_t *ht_item_new_key_val(const char *key, const char *val);
void ht_item_free(ht_item_t *it);

typedef struct ht {
  size_t cap;
  size_t len;
  ht_item_t **vals;
} ht_t;

ht_t *ht_new(void);
void ht_free(ht_t *ht);

#endif
