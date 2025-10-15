#include "inc_dec_sequences_count.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static uint64_t increasing_n(uint32_t v, int32_t level, uint64_t **cache);
static uint64_t decreasing_n(uint32_t v, int32_t level, uint64_t **cache);
uint64_t total_inc_dec(uint32_t n);

uint64_t increasing_n(uint32_t v, int32_t level, uint64_t **cache) {
  if (level < 0)
    return 1;

  if (cache[level][v])
    return cache[level][v];

  uint64_t sum = 0;
  for (uint32_t c = v; c <= 9; ++c) {
    sum += increasing_n(c, level - 1, cache);
  }
  return cache[level][v] = sum;
}
//////////////////////////////////////////////////////////////

uint64_t decreasing_n(uint32_t v, int32_t level, uint64_t **cache) {
  if (level < 0)
    return 1;

  if (cache[level][v])
    return cache[level][v];

  uint64_t sum = 0;
  for (int32_t c = v; c >= 0; --c) {
    sum += decreasing_n(c, level - 1, cache);
  }
  return cache[level][v] = sum;
}
//////////////////////////////////////////////////////////////

uint64_t total_inc_dec(uint32_t n) {
  if (n == 0)
    return 1;

  uint64_t **inc_cache = malloc(sizeof(uint64_t *) * n);
  uint64_t **dec_cache = malloc(sizeof(uint64_t *) * n);
  for (uint32_t i = 0; i < n; ++i) {
    inc_cache[i] = malloc(sizeof(uint64_t) * 10);
    dec_cache[i] = malloc(sizeof(uint64_t) * 10);
    memset(inc_cache[i], 0, sizeof(uint64_t) * 10);
    memset(dec_cache[i], 0, sizeof(uint64_t) * 10);
  }

  uint64_t inc_n = increasing_n(0, n - 1, inc_cache);
  uint64_t dec_n = 0;
  for (uint32_t l = 0; l < n; ++l) {
    dec_n += decreasing_n(9, l, dec_cache);
  }

  uint64_t res = inc_n + dec_n - 10 * n;
  for (uint32_t i = 0; i < n; ++i) {
    free(inc_cache[i]);
    free(dec_cache[i]);
  }
  free(inc_cache);
  free(dec_cache);

  return res;
}
//////////////////////////////////////////////////////////////
