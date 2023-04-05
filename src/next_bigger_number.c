#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "next_bigger_number.h"

static int ch_cmp(const void *l, const void *r) {
  char cl = *((char *)l);
  char cr = *((char *)r);
  return cl - cr;
}
//////////////////////////////////////////////////////////////

long long next_bigger_number(long long n) {
  char str[19] = {0};
  int slen = sprintf(str, "%llu", n);
  char *rh = str + slen - 1; // right half
  while (rh - 1 >= str && *rh <= *(rh - 1))
    --rh;

  if (rh == str) {
    return -1;
  }

  size_t rh_len = slen - (rh - str);
  qsort((void *)rh, rh_len, sizeof(char), ch_cmp);

  char *ptr_to_swap = rh;
  while (*ptr_to_swap && *ptr_to_swap <= *(rh - 1))
    ++ptr_to_swap;

  // swap
  char tmp = *(rh - 1);
  *(rh - 1) = *ptr_to_swap;
  *ptr_to_swap = tmp;

  return strtoll(str, NULL, 10);
}
//////////////////////////////////////////////////////////////
