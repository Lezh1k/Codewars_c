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

  pol_t p = pol_from_str("-1234567812345678");
  for (uint32_t i = 0; i < p.len; ++i)
    printf("%08u\n", p.data[i]);

  char *pstr = pol_to_str(&p);
  printf("%s\n", pstr);
  free(pstr);
}
