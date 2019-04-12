#ifndef KARATSUBA_H
#define KARATSUBA_H

#include <stddef.h>
#include <stdint.h>

typedef enum pol_sing {
  pol_negative = -1,
  pol_positive = 1
} pol_sign_t;
///////////////////////////////////////////////////////

/*
static uint64_t max_chars_per_pol_it(uint64_t bits,
                                     uint64_t base) {
  uint64_t t = 1ul << bits;
  uint64_t res = 0;
  while (t) {
    t /= base;
    ++res;
  }
  return res;
}
// max_chars_per_pol_it(sizeof(pol_item_t)*8, 10)
*/
typedef uint16_t pold_t;
#define POLD_NUM_BITS (sizeof(pol_item_t)*8)
#define POLD_MAX_DEC_LEN 4

typedef struct pol {
  pold_t *data;
  uint32_t alen;
  uint32_t len;
  int8_t sign;
} pol_t;

pol_t pol_new(uint32_t len);
void pol_free(pol_t *p);

pol_t pol_from_str(const char *str);
const char *pol_to_str(const pol_t* p);

#endif // KARATSUBA_H
