#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "next_smaller_number.h"

static int cmp_char(const void *l,
                    const void *r) {
  const char *lc = (const char*)l;
  const char *rc = (const char*)r;
  return *rc - *lc;
}

long long next_smaller_number(unsigned long long n){
  char str[100] = {0};
  int len = snprintf(str, 100, "%llu", n);

  int l_ix = len - 1;
  for (; l_ix >= 0; --l_ix) {
    bool found = false;
    for (int i = l_ix; i < len && !found; ++i) {
      found = str[i] < str[l_ix];
    }
    if (found) break; // do not want to decrement l_ix
  }

  if (l_ix == -1)
    return -1;

  int r_ix = l_ix+1;
  for (int i = r_ix; i < len; ++i) {
    if (str[i] >= str[l_ix]) continue;
    if (str[i] <= str[r_ix]) continue;
    r_ix = i;
  }

  char tmp = str[r_ix];
  str[r_ix] = str[l_ix];
  str[l_ix] = tmp;
  qsort(&str[l_ix+1], len-(l_ix+1), sizeof(char), cmp_char);
  if (str[0] == '0')
    return -1;
  return strtoll(str, NULL, 10);
}
