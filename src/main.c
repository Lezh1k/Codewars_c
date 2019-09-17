#include <stdint.h>
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "triangle.h"

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  const char *tests[] = {
    "B", "GB", "RRR",
    "RGBG", "RBRGBRB",
    "RBRGBRBGGRRRBGBBBGG",
    NULL
  };
  const char exp[] = {'B', 'R', 'R', 'B', 'G', 'G'};
  const char **t = tests;
  const char *e = exp;
  for (; *t; ++t, ++e) {
    if (*e != triangle(*t)) {
      printf("PIZDEZ\n");
      exit(1);
    }
  }
  printf("Passed!\n");
}
