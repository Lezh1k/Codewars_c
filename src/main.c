#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#include "bignum_mul.h"

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  char *mul = multiply("12", "3334");
  printf("%s\n", mul);
  free(mul);
}
