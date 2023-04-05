#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "palindrome_numbers.h"

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;
  printf("\n STARTING PROGRAM \n");
  ull_t ns[] = {1, 2, 10, 11, 28, 100, 1000, 19876, 61, 9206, 93006, 10230, 0};

  for (ull_t *pn = ns; *pn != 0; ++pn) {
    ull_t dst = find_reverse_number(*pn);
    printf("%llu\n", dst);
  }

  return 0;
}
//////////////////////////////////////////////////////////////

