#include "next_bigger_number.h"
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;
  int nums[] = {3792, 10990, -1};
  // expected 3927, 19009
  for (int *n = nums; *n != -1; ++n) {
    next_bigger_number(*n);
  }
  return 0;
}
//////////////////////////////////////////////////////////////
