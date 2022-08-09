#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lcs.h"

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;
  char *lsc_str = lcs("abcdef", "abdfa");
  printf("%s\n", lsc_str);
  free(lsc_str);
  return 0;
}
////////////////////////////////////////////////////////////


