#include <stdint.h>
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "bignum.h"

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  setbuf(stdout, NULL);

  pol_t a = pol_from_str("99999999990");
  pol_t b = pol_from_str("22222222220");
  pol_t c = pol_mul_karatsuba(&a, &b);

  char *pstr = pol_to_str(&c);
  printf("%s\n", pstr);
  free(pstr);

  pol_free(&a);
  pol_free(&b);
  pol_free(&c);
}
