#include <stdio.h>
#include <stdlib.h>
#include <nmmintrin.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <ctype.h>
#include "karatsuba.h"

uint32_t powi(uint32_t base, uint32_t exp) {
  uint32_t res = 1;
  while (exp) {
    if (exp & 1)
      res *= base;
    exp >>= 1;
    base *= base;
  }
  return res;
}
///////////////////////////////////////////////////////

static uint32_t str2ui(const char *str, uint32_t len) {
  uint32_t p = powi(10, len-1);
  uint32_t r = 0;
  for (; len; --len, p /= 10)
    r += (uint32_t)(*str++ - '0') * p;
  return r;
}
///////////////////////////////////////////////////////

pol_t pol_new(uint32_t len, pol_sign_t sign) {
  pol_t p = {.len = len, .sign = (int8_t)sign, .data = NULL};
  if (len) {
    p.data = calloc(p.len, sizeof(pol_limb_t));
    assert(p.data);
  }
  return p;
}
///////////////////////////////////////////////////////

void pol_free(pol_t *p) {
  if (p->data)
    free(p->data);
}
///////////////////////////////////////////////////////

pol_t pol_from_str(const char *str) {  
  uint8_t c = (uint8_t) *str;
  while (isspace(c))
    c = (uint8_t) *++str;
  if (!c)
    return pol_new(0, pols_pos);

  int8_t sign = 1;
  if (*str == '+')
    ++str;
  if (*str == '-') {
    sign = -1;
    ++str;
  }

  while (*str == '0') ++str;
  if (!*str)
    return pol_new(0, pols_pos);

  div_t d = div((int)strlen(str), POL_BASE_DIGITS);
  int lcnt = d.quot + (d.rem ? 1 : 0);
  int fll = d.rem;

  pol_t r = pol_new((uint32_t)lcnt, sign);
  pol_limb_t *ptr = &r.data[lcnt-1];

  if (fll) {
    *ptr-- = (pol_limb_t) str2ui(str, (uint32_t)fll);
    str += fll;
  }

  while (ptr >= r.data) {
    *ptr-- = (pol_limb_t) str2ui(str, POL_BASE_DIGITS);
    str += POL_BASE_DIGITS;
  }
  return r;
}
///////////////////////////////////////////////////////

char *pol_to_str(const pol_t *p) {
  char *rbuff, *tmp;
  if (!p->len) {
    rbuff = malloc(1);
    *rbuff = 0;
    return rbuff;
  }

  rbuff = malloc(p->len * POL_BASE_DIGITS + 2); //for \0 at the end of string and sign
  tmp = rbuff;
  if (p->sign == pols_neg)
    *rbuff++ = '-';

  pol_limb_t *ptr = &p->data[p->len-1];
  int offset = 0;

  if (*ptr)
    offset = sprintf(rbuff, "%u", *ptr);
  --ptr;

  while (ptr >= p->data) {
    rbuff += offset;
    offset = sprintf(rbuff, POL_LIMB_FORMAT, *ptr--);
  }
  return tmp;
}
///////////////////////////////////////////////////////

pol_t pol_sum(const pol_t *l,
              const pol_t *r) {
  if (l->sign != r->sign)
    return pol_sub(l, r);
  return pol_new(0, l->sign);
}
///////////////////////////////////////////////////////

pol_t pol_sub(const pol_t *l,
              const pol_t *r) {
  return pol_new(0, pols_neg); //todo implement
}
///////////////////////////////////////////////////////
