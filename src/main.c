#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "spinning_rings.h"


int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  // basic tests
  dotest(2, 2, 3);
  dotest(5, 5, 3);
  dotest(2, 10, 13);
  dotest(10, 2, 10);
  dotest(7, 9, 4);
  dotest(1, 1, 1);
  dotest(16777216, 14348907, 23951671);

  // random tests
  dotest(7571, 3580, 7367);

  printf("%lld\n", spinning_rings(88399884230861, 62348759298410));
  printf("%lld\n", spinning_rings(92761874409660, 159281049706940));
  return 0;
}
//////////////////////////////////////////////////////////////
