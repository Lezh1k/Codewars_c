#include <stdio.h>
#include <stdlib.h>
#include <nmmintrin.h>
#include <string.h>
#include <assert.h>

#include "karatsuba.h"

typedef struct pol_pair {
  pol_t *big;
  pol_t *sml;
} pol_pair_t;

static pol_pair_t get_pair(pol_t *x, pol_t *y);


pol_t pol_new(size_t len) {
  pol_t r = {.len = len};
  r.data = calloc(r.len, sizeof(pol_item_t));
  return r;
}
///////////////////////////////////////////////////////

void pol_free(pol_t p) {
  free(p.data);
  p.data = NULL;
}
///////////////////////////////////////////////////////

pol_t pol_from_str(const char *str) {
  pol_t r = pol_new(strlen(str));
  pol_item_t *dst;
  dst = r.data + r.len - 1;
  for (; *str; ++str)
    *dst-- = *str - '0';
  return r;
}
///////////////////////////////////////////////////////

pol_pair_t get_pair(pol_t *x, pol_t *y) {
  pol_t *t;
  pol_pair_t res = {.big = x, .sml = y};
  if (x->len < y->len) {
    t = res.big;
    res.big = res.sml;
    res.sml = t;
  }
  return res;
}
///////////////////////////////////////////////////////

void pol_pow10(pol_t *p, size_t n) {
  pol_item_t *dst, *src;
  p->data = realloc(p->data, p->len+n);
  src = p->data + p->len - 1;
  dst = src + n;
  while (src >= p->data)
    *dst-- = *src--;
  while (dst >= p->data)
    *dst-- = 0;
  p->len += n;
}
///////////////////////////////////////////////////////

void pol_print(pol_t p) {
  while (p.len--)
    printf("%d", *p.data++);
}
///////////////////////////////////////////////////////

pol_t pol_sum(pol_t l, pol_t r) {
  pol_pair_t pair = get_pair(&l, &r);
  pol_t res = pol_new(pair.big->len+1);
  int8_t carry = 0;
  size_t i;
  for (i = 0; i < pair.sml->len; ++i) {
    res.data[i] = pair.sml->data[i] + pair.big->data[i] + carry;
    carry = res.data[i] >= 10;
    res.data[i] %= 10;
  }

  for (; i < pair.big->len; ++i) {
    res.data[i] = pair.big->data[i] + carry;
    carry = res.data[i] >= 10;
    res.data[i] %= 10;
  }

  if (!carry) {
    --res.len;
    return res;
  }

  res.data[i] = carry; //cause big.len = res.len - 1
  return res;
}
///////////////////////////////////////////////////////

pol_t pol_sub(pol_t l, pol_t r) {
  assert(l.len >= r.len);
  pol_t res = pol_new(l.len);
  int8_t carry = 0;
  size_t i;
  for (i = 0; i < r.len; ++i) {
    res.data[i] = l.data[i] - r.data[i] - carry;
    carry = res.data[i] < 0;
    if (carry)
      res.data[i] += 10;
  }

  for (; i < l.len; ++i) {
    res.data[i] = l.data[i] - carry;
    carry = res.data[i] < 0;
    if (carry)
      res.data[i] += 10;
  }
  return res;
}
///////////////////////////////////////////////////////

pol_t pol_naive_mul(pol_t l, pol_t r) {
  pol_t res = pol_new(l.len + r.len);
  size_t i, j;
  int8_t carry;
  for (i = 0; i < l.len; ++i) {
    carry = 0;
    for (j = 0; j < r.len; ++j) {
      res.data[i+j] += carry + l.data[i] * r.data[j];
      carry = res.data[i+j] / 10;
      res.data[i+j] %= 10;
    }
    res.data[i+j] += carry;
  }
  return res;
}
///////////////////////////////////////////////////////

static void pol_complement(pol_t *x, pol_t *y) {
  pol_t **px, **py;
  if (x->len == y->len) return;
  px = &x;
  py = &y;
  if (y->len > x->len) {
    //swap
    px = &y;
    py = &x;
  }

  pol_item_t *tmp = calloc((*px)->len, sizeof(pol_item_t));
  memcpy(tmp, (*py)->data, (*py)->len*sizeof(pol_item_t));
  free((*py)->data);
  (*py)->data = tmp;
  (*py)->len = (*px)->len;
}
///////////////////////////////////////////////////////

static void pol_normalize_len(pol_t *p) {
  pol_item_t *end = p->data + p->len - 1;
  while (!*end && end >= p->data) {
    --p->len;
    --end;
  }
}
///////////////////////////////////////////////////////

/*
Karatsuba_mul(X, Y):
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
    вернуть Prod1 * 10 ^ n + (Prod3 - Prod1 - Prod2) * 10 ^ (n / 2) + Prod2
*/
#define NAIVE_THRESHOLD 2
pol_t pol_karatsuba_mul(pol_t x, pol_t y) {
  size_t n = x.len > y.len ? x.len : y.len;
  pol_t xl, xr, yl, yr;
  pol_t p1, p2, p3;
  pol_t xlr, ylr;
  pol_t p31, p312;
  pol_t s1;
  pol_t res;

  //todo check threshold
  if (n <= NAIVE_THRESHOLD)
    return pol_naive_mul(x, y);

  pol_complement(&x, &y);

  yl.len = xl.len = n / 2;
  yr.len = xr.len = x.len - xl.len;
  xr.data = x.data;
  xl.data = x.data + xr.len;
  yr.data = y.data;
  yl.data = y.data+yr.len;

  p1 = pol_karatsuba_mul(xl, yl);
  p2 = pol_karatsuba_mul(xr, yr);
  xlr = pol_sum(xl, xr);
  ylr = pol_sum(yl, yr);
  p3 = pol_karatsuba_mul(xlr, ylr);
  p31 = pol_sub(p3, p1);
  p312 = pol_sub(p31, p2);

  pol_normalize_len(&p2);
  pol_normalize_len(&p312);
  pol_normalize_len(&p1);

  pol_pow10(&p1, n);
  pol_pow10(&p312, n - n/2);
  s1 = pol_sum(p1, p312);
  res = pol_sum(s1, p2);

  pol_free(xlr);
  pol_free(ylr);
  pol_free(p1);
  pol_free(p2);
  pol_free(p3);
  pol_free(p31);
  pol_free(p312);
  pol_free(s1);

  return res;
}
///////////////////////////////////////////////////////
