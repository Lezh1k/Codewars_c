#include <stdio.h>
#include <stdlib.h>
#include <nmmintrin.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <ctype.h>
#include "karatsuba.h"

typedef struct pol_pairc {
  const pol_t *big;
  const pol_t *small;
} pol_pairc_t;

static uint32_t str2ui(const char *str, uint32_t len);
static uint32_t powi(uint32_t base, uint32_t exp);
static pol_pairc_t pol_sum_pairc(const pol_t *l, const pol_t *r);

//remove leading zeros
static void pol_trim(pol_t *p);

pol_pairc_t pol_sum_pairc(const pol_t *l, const pol_t *r) {
  pol_pairc_t p = {.big = l, .small = r};
  if (l->len < r->len) {
    p.big = r;
    p.small = l;
  }
  return p;
}
///////////////////////////////////////////////////////

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

uint32_t str2ui(const char *str, uint32_t len) {
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
  char *rbuff, *result;
  if (!p->len) {
    rbuff = malloc(2);
    *rbuff = '0';
    *(rbuff+1) = 0;
    return rbuff;
  }

  rbuff = malloc(p->len * POL_BASE_DIGITS + 2); //for \0 at the end of string and sign
  result = rbuff;
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
  return result;
}
///////////////////////////////////////////////////////

pol_t pol_sum(const pol_t *l,
              const pol_t *r) {
  if (l->sign != r->sign) {
    pol_t ir = pol_inv(r);
    return pol_sub(l, &ir);
  }

  pol_pairc_t pair = pol_sum_pairc(l, r);
  pol_t res = pol_new(pair.big->len + 1, pair.big->sign);
  pol_limb_t carry = 0;
  uint32_t i;
  for (i = 0; i < pair.small->len; ++i) {
    res.data[i] = pair.big->data[i] + pair.small->data[i] + carry;
    carry = res.data[i] >= POL_BASE;
    if (carry)
      res.data[i] -= POL_BASE;
  }

  for (; i < pair.big->len; ++i) {
    res.data[i] = pair.big->data[i] + carry;
    carry = res.data[i] >= POL_BASE;
    if (carry)
      res.data[i] -= POL_BASE;
  }
  if (carry)
    res.data[i++] = carry;
  res.len = i;

  return res;
}
///////////////////////////////////////////////////////

pol_t pol_sub(const pol_t *l,
              const pol_t *r) {
  if (l->sign != r->sign) {
    pol_t ir = pol_inv(r);
    return pol_sum(l, &ir);
  }

  pol_t la, ra;
  la = pol_abs(l);
  ra = pol_abs(r);
  if (pol_cmp(&la, &ra) < 0) {
    pol_t tmp = pol_sub(r, l);
    return pol_inv(&tmp);
  }

  pol_t res = pol_new(l->len, l->sign);
  pol_limb_t carry = 0;
  uint32_t i;
  for (i = 0; i < r->len; ++i) {
    res.data[i] = l->data[i] - r->data[i] - carry;
    carry = res.data[i] < 0;
    if (carry)
      res.data[i] += POL_BASE;
  }

  for (; i < l->len; ++i) {
    res.data[i] = l->data[i] - carry;
    carry = res.data[i] < 0;
    if (carry)
      res.data[i] += POL_BASE;
  }
  pol_trim(&res);
  return res;
}
///////////////////////////////////////////////////////

pol_t pol_inv(const pol_t *p) {
  pol_t r = {.data = p->data, .len = p->len, .sign = -p->sign};
  return r;
}
///////////////////////////////////////////////////////

pol_t pol_abs(const pol_t *p) {
  pol_t r = {.data = p->data, .len = p->len, .sign = pols_pos};
  return r;
}
///////////////////////////////////////////////////////

int pol_cmp(const pol_t *l,
            const pol_t *r) {
  if (l->sign != r->sign)
    return l->sign - r->sign;
  if (l->len != r->len)
    return (int)l->len*l->sign - (int)r->len*r->sign;
  for (int i = (int)l->len-1; i >= 0; --i) {
    if (l->data[i] != r->data[i])
      return l->data[i]*l->sign - r->data[i]*r->sign;
  }
  return 0; //equals
}
///////////////////////////////////////////////////////

void pol_trim(pol_t *p) {
  while (p->len && !p->data[p->len-1])
    --p->len;
}
///////////////////////////////////////////////////////

pol_t pol_mul(const pol_t *l, const pol_t *r) {
  uint32_t rn = l->len + r->len;
  pol_t res = pol_new(rn, l->sign * r->sign);
  int32_t carry = 0;
  for (uint32_t i=0; i < l->len; ++i) {
    for (uint32_t j=0; j < r->len || carry; ++j) {
      int64_t cur = res.data[i+j] + (int64_t)l->data[i] * (j < r->len ? r->data[j] : 0) + carry;
      res.data[i+j] = (int32_t)(cur % POL_BASE);
      carry = (int32_t) (cur / POL_BASE);
    }
  }

  pol_trim(&res);
  return res;
}
///////////////////////////////////////////////////////

pol_t pol_mul_karatsuba(const pol_t *l, const pol_t *r) {
  return pol_new(0, l->sign*r->sign);
}
///////////////////////////////////////////////////////
