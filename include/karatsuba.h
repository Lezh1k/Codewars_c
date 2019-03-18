#ifndef KARATSUBA_H
#define KARATSUBA_H

#include <stddef.h>
#include <stdint.h>

typedef int8_t pol_item_t;
typedef struct pol {
  pol_item_t *data;
  uint32_t len;
} pol_t;

pol_t pol_new(uint32_t len);
void pol_free(pol_t p);
pol_t pol_from_str(const char *str);

pol_t pol_sum(pol_t l,
              pol_t r);

pol_t pol_sub(pol_t l,
              pol_t r);

pol_t pol_karatsuba_mul(pol_t *x,
                        pol_t *y);


void pol_print(pol_t p);
#endif // KARATSUBA_H
