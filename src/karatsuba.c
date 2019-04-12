#include <stdio.h>
#include <stdlib.h>
#include <nmmintrin.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <ctype.h>
#include "karatsuba.h"

pol_t pol_new(uint32_t len) {
  pol_t p = {.alen = len, .len = 0};
  p.data = calloc(p.alen, sizeof(pold_t));
  if (p.alen)
    assert(p.data);
  return p;
}
///////////////////////////////////////////////////////

void pol_free(pol_t *p) {
  free(p->data);
}
///////////////////////////////////////////////////////

pol_t pol_from_str(const char *str) {
  size_t dn, ps;
  uint8_t c = (uint8_t) *str;
  while (isspace(c))
    c = (uint8_t) *++str;
  if (*str == 0)
    return pol_new(0);  
  dn = strlen(str);
  ps = dn / POLD_MAX_DEC_LEN;

  /*
  for (int i=(int)strlen(s); i>0; i-=9) {
  s[i] = 0;
  a.push_back (atoi (i>=9 ? s+i-9 : s));
}
*/
  return pol_new(0);
}
///////////////////////////////////////////////////////

const char *pol_to_str(const pol_t *p) {
  return "";
}
