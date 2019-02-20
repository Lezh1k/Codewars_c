#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
#include <emmintrin.h>
#include "karatsuba.h"

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  pol_t x = pol_from_str("999");
  pol_t y = pol_from_str("99");
  pol_t z = pol_karatsuba_mul(x, y);

  pol_print(z);
  printf("\n");
  printf("%d\n", 999*99);

  pol_free(x);
  pol_free(y);
  pol_free(z);

  return 0;
}
///////////////////////////////////////////////////////
