#include "prime_stream_simple.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct prime_stream {
  uint64_t *primes;
  uint64_t N;
  uint32_t curr_i;
} prime_stream_t;

static void eratosthenes_sieve(uint64_t *arr, uint64_t N) {
  arr[0] = arr[1] = false;
  for (uint64_t i = 2; i * i <= N; ++i) {
    if (!arr[i])
      continue;

    for (uint64_t j = i * i; j <= N; j += i) {
      arr[j] = false;
    }
  }
}
/* Allocate and initialize a new stream */
prime_stream_t *prime_stream_new(void) {
  static const uint64_t initial_N = 100000;
  prime_stream_t *ps = malloc(sizeof(prime_stream_t));
  ps->N = initial_N;
  ps->primes = malloc(sizeof(uint64_t) * ps->N);
  ps->curr_i = 0;
  memset(ps->primes, 0x1, sizeof(uint64_t) * ps->N);
  eratosthenes_sieve(ps->primes, ps->N - 1);
  return ps;
}

/* Get the next prime in the stream */
unsigned prime_stream_next(prime_stream_t *ps) {
  // actually I don't want recursion here
  const int recursion_depth_max = 2;
  for (int i = 0; i < recursion_depth_max; ++i) {
    for (; ps->curr_i < ps->N; ++ps->curr_i) {
      if (ps->primes[ps->curr_i]) {
        return ps->curr_i++; // hack
      }
    }

    // if we are here this means we reached N. SO!
    perror("reached N. realloc called\n");
    ps->primes = realloc(ps->primes, sizeof(uint64_t) * (ps->N * 2));
    memset(&ps->primes[ps->N], 0x1, sizeof(uint64_t) * ps->N);
    ps->N *= 2;
    eratosthenes_sieve(ps->primes, ps->N - 1);
  }

  // if we are here - we reached max recursion depth
  // so sad
  return -1; // probably it's better to
}

/* Free your prime stream */
void prime_stream_destroy(prime_stream_t *ps) {
  if (ps && ps->primes) {
    free(ps->primes);
  }
  free(ps);
}
//////////////////////////////////////////////////////////////
