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

  bn_t a = bn_from_str("99999999990");
  bn_t b = bn_from_str("22222222220");
  bn_t c = bn_mul_karatsuba(&a, &b);

  char *pstr = bn_to_str(&c);
  printf("%s\n", pstr);
  free(pstr);

  bn_free(&a);
  bn_free(&b);
  bn_free(&c);
}
