#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
#include <emmintrin.h>
#include "karatsuba.h"
#include "brainfuck.h"

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  pol_t x = pol_from_str("20000000");
  pol_t y = pol_from_str("50000000");
  pol_t z = pol_karatsuba_mul(&x, &y);

  pol_print(z);
  printf("\n");

  pol_free(x);
  pol_free(y);
  pol_free(z);

  return 0;
}
/////////////////////////////////////////////////////////
