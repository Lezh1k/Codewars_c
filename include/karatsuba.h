#ifndef KARATSUBA_H
#define KARATSUBA_H

#include <stddef.h>
#include <stdint.h>

typedef enum pol_sing {
  pol_negative = -1,
  pol_positive = 1
} pol_sign_t;
///////////////////////////////////////////////////////

typedef uint16_t pold_t;
#define POLD_NUM_BITS (sizeof(pol_item_t)*8)
#define POLD_MAX_DEC_LEN 4

typedef struct pol {
  pold_t *data;  
  uint32_t len;
  int8_t sign;
} pol_t;

pol_t pol_new(uint32_t len);
void pol_free(pol_t *p);

pol_t pol_from_str(const char *str);
const char *pol_to_str(const pol_t* p);

#endif // KARATSUBA_H
