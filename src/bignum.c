#include <stdio.h>
#include <stdlib.h>
#include <nmmintrin.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <ctype.h>
#include "bignum.h"

typedef struct bn_pairc {
  const bn_t *big;
  const bn_t *small;
} bn_pairc_t;

static uint32_t str2ui(const char *str, uint32_t len);
static uint32_t powi(uint32_t base, uint32_t exp);
static uint32_t nearest_power_of_2(uint32_t v);

static void bn_increase_len(bn_t *p, uint32_t l);
static bn_t bn_part(const bn_t *src, uint32_t start, uint32_t len);
static void bn_karatsuba_prepare(bn_t *l, bn_t *r);
static bn_t bn_mul_karatsuba_internal(bn_t *l, bn_t *r);
static void bn_trim(bn_t *p);//remove leading zeros


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

bn_t bn_new(uint32_t len, bn_sign_t sign) {
  bn_t p = {.len = len, .sign = (int8_t)sign, .data = NULL};
  if (len) {
    p.data = calloc(p.len, sizeof(bn_word_t));
    assert(p.data);
  }
  return p;
}
///////////////////////////////////////////////////////

void bn_free(bn_t *p) {
  if (p->data)
    free(p->data);
}
///////////////////////////////////////////////////////

bn_t bn_from_str(const char *str) {
  uint8_t c = (uint8_t) *str;
  while (isspace(c))
    c = (uint8_t) *++str;
  if (!c)
    return bn_new(0, bns_pos);

  int8_t sign = 1;
  if (*str == '+')
    ++str;
  if (*str == '-') {
    sign = -1;
    ++str;
  }

  while (*str == '0') ++str;
  if (!*str)
    return bn_new(0, bns_pos);

  div_t d = div((int)strlen(str), BN_BASE_DIGITS);
  int lcnt = d.quot + (d.rem ? 1 : 0);
  int fll = d.rem;

  bn_t r = bn_new((uint32_t)lcnt, sign);
  bn_word_t *ptr = &r.data[lcnt-1];

  if (fll) {
    *ptr-- = (bn_word_t) str2ui(str, (uint32_t)fll);
    str += fll;
  }

  while (ptr >= r.data) {
    *ptr-- = (bn_word_t) str2ui(str, BN_BASE_DIGITS);
    str += BN_BASE_DIGITS;
  }
  return r;
}
///////////////////////////////////////////////////////

char *bn_to_str(const bn_t *p) {
  char *rbuff, *result;
  if (!p->len) {
    rbuff = malloc(2);
    *rbuff = '0';
    *(rbuff+1) = 0;
    return rbuff;
  }

  rbuff = calloc(1, p->len * BN_BASE_DIGITS + 2);
  result = rbuff;
  if (p->sign == bns_neg)
    *rbuff++ = '-';
  *rbuff = '0';

  bn_word_t *ptr = (bn_word_t*) &p->data[p->len-1];

  //don't print heading zeros
  while (ptr >= p->data && *ptr == 0)
    --ptr;

  if (ptr < p->data)
    return result;

  int offset = sprintf(rbuff, "%u", *ptr--);
  while (ptr >= p->data) {
    rbuff += offset;
    offset = sprintf(rbuff, BN_WORD_FORMAT, *ptr--);
  }
  return result;
}
///////////////////////////////////////////////////////

bn_t bn_sum(const bn_t *l,
            const bn_t *r) {
  const bn_t *big, *small;
  big = l; small = r;
  if (r->len > l->len) {
    big = r; small = l;
  }

  if (l->sign != r->sign) {
    bn_t ir = bn_inv(r);
    return bn_sub(l, &ir);
  }

  bn_t res = bn_new(big->len + 1, big->sign);
  bn_word_t carry = 0;
  uint32_t i;
  for (i = 0; i < small->len; ++i) {
    res.data[i] = big->data[i] + small->data[i] + carry;
    carry = res.data[i] >= BN_BASE;
    if (carry)
      res.data[i] -= BN_BASE;
  }

  for (; i < big->len; ++i) {
    res.data[i] = big->data[i] + carry;
    carry = res.data[i] >= BN_BASE;
    if (carry)
      res.data[i] -= BN_BASE;
  }

  if (carry)
    res.data[i++] = carry;
  res.len = i;
  return res;
}
///////////////////////////////////////////////////////

bn_t bn_sub(const bn_t *l,
            const bn_t *r) {
  if (l->sign != r->sign) {
    bn_t ir = bn_inv(r);
    return bn_sum(l, &ir);
  }

  bn_t la, ra;
  la = bn_abs(l);
  ra = bn_abs(r);
  if (bn_cmp(&la, &ra) < 0) {
    bn_t tmp = bn_sub(r, l);
    return bn_inv(&tmp);
  }

  bn_t res = bn_new(l->len, l->sign);
  bn_word_t carry = 0;
  uint32_t i;
  for (i = 0; i < r->len; ++i) {
    res.data[i] = l->data[i] - r->data[i] - carry;
    carry = res.data[i] < 0;
    if (carry)
      res.data[i] += BN_BASE;
  }

  for (; i < l->len; ++i) {
    res.data[i] = l->data[i] - carry;
    carry = res.data[i] < 0;
    if (carry)
      res.data[i] += BN_BASE;
  }
  bn_trim(&res);
  return res;
}
///////////////////////////////////////////////////////

bn_t bn_inv(const bn_t *p) {
  bn_t r = {.data = p->data, .len = p->len, .sign = -p->sign};
  return r;
}
///////////////////////////////////////////////////////

bn_t bn_abs(const bn_t *p) {
  bn_t r = {.data = p->data, .len = p->len, .sign = bns_pos};
  return r;
}
///////////////////////////////////////////////////////

int bn_cmp(const bn_t *l,
           const bn_t *r) {
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

void bn_trim(bn_t *p) {
  while (p->len && !p->data[p->len-1])
    --p->len;
}
///////////////////////////////////////////////////////

bn_t bn_mul_naive(const bn_t *l, const bn_t *r) {
  uint32_t rn = l->len + r->len;
  bn_t res = bn_new(rn, l->sign * r->sign);
  bn_word_t carry = 0;
  for (uint32_t i=0; i < l->len; ++i) {
    for (uint32_t j=0; j < r->len || carry; ++j) {
      bn_dword_t cur = res.data[i+j] + (bn_dword_t)l->data[i] * (j < r->len ? r->data[j] : 0) + carry;
      res.data[i+j] = (bn_word_t)(cur % BN_BASE);
      carry = (bn_word_t) (cur / BN_BASE);
    }
  }

  bn_trim(&res);
  return res;
}
///////////////////////////////////////////////////////

uint32_t nearest_power_of_2(uint32_t v) {
  --v;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  return ++v;
}
//////////////////////////////////////////////////////////////////////////

void bn_increase_len(bn_t *p, uint32_t l) {
  assert(p->len <= l);
  if (p->len == l) return;
  bn_word_t *tmp = malloc(l * sizeof(bn_word_t));
  memcpy(tmp, p->data, p->len*sizeof(bn_word_t));
  memset(&tmp[p->len], 0, (l - p->len)*sizeof(bn_word_t));
  free(p->data);
  p->data = tmp;
  p->len = l;
}
///////////////////////////////////////////////////////

void bn_karatsuba_prepare(bn_t *l, bn_t *r) {
  uint32_t ml = l->len > r->len ? l->len : r->len;
  ml = nearest_power_of_2(ml);
  if (l->len != ml)
    bn_increase_len(l, ml);
  if (r->len != ml)
    bn_increase_len(r, ml);
}
///////////////////////////////////////////////////////

bn_t bn_part(const bn_t *src, uint32_t start, uint32_t len) {
  bn_t res = bn_new(len, src->sign);
  memcpy(res.data, &src->data[start], len * sizeof (bn_word_t));
  return res;
}
///////////////////////////////////////////////////////

/*
 * Karatsuba_mul(X, Y):
    // X, Y - целые числа длины n
    n = max(размер X, размер Y)
    если n = 1: вернуть X * Y
    X_l = левые n/2 цифр X
    X_r = правые n/2 цифр X
    Y_l = левые n/2 цифр Y
    Y_r = правые n/2 цифр Y
    Prod1 = Karatsuba_mul(X_l, Y_l)
    Prod2 = Karatsuba_mul(X_r, Y_r)
    Prod3 = Karatsuba_mul(X_l + X_r, Y_l + Y_r)
    return Prod1 * base ^ n + (Prod3 - Prod1 - Prod2) * base ^ (n / 2) + Prod2
*/

bn_t bn_mul_karatsuba_internal(bn_t *l,
                               bn_t *r) {
  uint32_t n = l->len;
  if (n == 1) {
    bn_t res = bn_new(2, l->sign * r->sign);
    bn_dword_t cur = (bn_dword_t)l->data[0] * (bn_dword_t)(r->data[0]);
    res.data[0] = (bn_word_t)(cur % BN_BASE);
    res.data[1] = (bn_word_t)(cur / BN_BASE);
    bn_trim(&res);
    return res;
  }

  bn_t res = bn_new(n+n, l->sign*r->sign);
  uint32_t k = n >> 1;

  bn_t xr = bn_part(l, 0, k);
  bn_t xl = bn_part(l, k, k);
  bn_t yr = bn_part(r, 0, k);
  bn_t yl = bn_part(r, k, k);

  bn_t p1 = bn_mul_karatsuba_internal(&xl, &yl);
  bn_t p2 = bn_mul_karatsuba_internal(&xr, &yr);

  for (uint32_t i = 0; i < k; ++i) {
    xl.data[i] += xr.data[i];
    yl.data[i] += yr.data[i];
  }

  bn_t p3 = bn_mul_karatsuba_internal(&xl, &yl);

  //p3 - p1 - p2
  for (uint32_t i = 0; i < p1.len; ++i)
    p3.data[i] -= p1.data[i];
  for (uint32_t i = 0; i < p2.len; ++i)
    p3.data[i] -= p2.data[i];
  //

  //res = p1 * base^n + (p3-p1-p2) * base^k + p2
  //(p3-p1-p2) * base^k
  for (uint32_t i = 0; i < p3.len; ++i)
    res.data[i+k] += p3.data[i];

  //p1 * base^n
  for (uint32_t i = 0; i < p1.len; ++i)
    res.data[i+n] += p1.data[i];

  //p2
  for (uint32_t i = 0; i < p2.len; ++i)
    res.data[i] += p2.data[i];


  //normalization of negative values. kind of hack here :)
  bn_word_t carry = 0;
  for (uint32_t i = 0; i < res.len; ++i) {
    res.data[i] = res.data[i] - carry;
    carry = 0;
    while (res.data[i] < 0) {
      ++carry;
      res.data[i] += BN_BASE;
    }
  }

  //don't forget about memory. seems like it's possible
  //to optimize this part and use pointers to temp. results.
  bn_free(&xl);
  bn_free(&xr);
  bn_free(&yl);
  bn_free(&yr);
  bn_free(&p1);
  bn_free(&p2);
  bn_free(&p3);

  return res;
}
///////////////////////////////////////////////////////

bn_t bn_mul_karatsuba(bn_t *l, bn_t *r) {
  bn_karatsuba_prepare(l, r);
  bn_t tmp = bn_mul_karatsuba_internal(l, r);
  bn_t res = bn_new(l->len + r->len, l->sign * r->sign);
  bn_word_t carry = 0;
  for (uint32_t i = 0; i < tmp.len; ++i) {
    bn_dword_t cur = (bn_dword_t) (tmp.data[i] + carry);
    res.data[i] = cur % BN_BASE;
    carry = (bn_word_t) (cur / BN_BASE);
  }
  bn_trim(&res);
  bn_free(&tmp);
  return res;
}
///////////////////////////////////////////////////////

void bn_print_raw(const bn_t *p) {
  for (uint32_t i = 0; i < p->len; ++i)
    printf("%d ", p->data[i]);
  printf("\n");
}
///////////////////////////////////////////////////////

void bn_print(const bn_t *p, const char *descr) {
  char *s = bn_to_str(p);
  printf("%s: %s\n", descr, s);
  free(s);
}
///////////////////////////////////////////////////////
