#include <stdint.h>
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "karatsuba.h"

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  pol_t a = pol_from_str("9999");
  pol_t b = pol_from_str("2");
  pol_t c = pol_mul(&a, &b);

  char *pstr = pol_to_str(&c);
  printf("%s\n", pstr);
  free(pstr);

  pol_free(&a);
  pol_free(&b);
  pol_free(&c);
}
