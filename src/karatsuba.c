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
static void pol_pow10(pol_t *p, size_t n);
static pol_t pol_naive_mul(const pol_t *l, const pol_t *r);
static uint32_t nearest2pow(uint32_t v);

pol_t pol_new(size_t len) {
  pol_t p = {.len = len};
  p.data = calloc(p.len, sizeof(pol_item_t));
  assert(p.data);
  return p;
}
///////////////////////////////////////////////////////

void pol_free(pol_t p) {
  free(p.data);
  p.data = NULL;
}
///////////////////////////////////////////////////////

uint32_t nearest2pow(uint32_t v) {
  --v;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  return ++v;
}
//////////////////////////////////////////////////////////////////////////

pol_t pol_from_str(const char *str) {
  uint32_t len = strlen(str);
  uint32_t alen = nearest2pow(len);
  pol_t p = pol_new(alen);
  pol_item_t *dst;
  dst = p.data + len - 1;
  for (; *str; ++str)
    *dst-- = *str - '0';
  return p;
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
  assert(p->data);
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
  printf("\n");
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
//  assert(l.len >= r.len);
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

pol_t pol_naive_mul(const pol_t *l,
                    const pol_t *r) {
  pol_t res = pol_new(l->len + r->len);
  size_t i, j;
  int8_t carry;
  for (i = 0; i < l->len; ++i) {
    carry = 0;
    for (j = 0; j < r->len; ++j) {
      res.data[i+j] += carry + l->data[i] * r->data[j];
      carry = res.data[i+j] / 10;
      res.data[i+j] %= 10;
    }
    res.data[i+j] += carry;
  }
  return res;
}
///////////////////////////////////////////////////////

static void pol_complement(pol_t *x, pol_t *y) {  
  pol_t *t;
  if (x->len == y->len) return;
  if (y->len > x->len) {
    t = x; x = y; y = x; //swap pointers
  }
  y->data = realloc(y->data, x->len*sizeof(pol_item_t));
  assert(y->data); //break if realloc failed
  memset(y->data + y->len, 0, (x->len - y->len) * sizeof(pol_item_t));
  y->len = x->len;
}
///////////////////////////////////////////////////////

static void pol_normalize_len(pol_t *p) {
  pol_item_t *end = p->data + p->len - 1;
  while (end >= p->data && !*end) {
    --p->len;
    --end;
  }
}
///////////////////////////////////////////////////////

static void pol_print_descr(const char *descr, pol_t p) {
  printf("%s:\t", descr);
  pol_print(p);
}
///////////////////////////////////////////////////////

pol_t pol_karatsuba_mul(pol_t *x, pol_t *y) {
  #define NAIVE_THRESHOLD 2
  size_t n = x->len > y->len ? x->len : y->len;
  pol_t xl, xr, yl, yr;
  pol_t p1, p2, p3;
  pol_t xlr, ylr;
  pol_t p31, p312;
  pol_t s1;
  pol_t res;

  printf("\n******************\n");
  pol_print_descr("x", *x);
  pol_print_descr("y", *y);
  //todo check threshold
  if (n <= NAIVE_THRESHOLD) {
    printf("naive\n");
    return pol_naive_mul(x, y);
  }

  pol_complement(x, y);
  pol_print_descr("x", *x);
  pol_print_descr("y", *y);

  xl = pol_new(n / 2);
  yl = pol_new(xl.len);
  xr = pol_new(n - xl.len);
  yr = pol_new(xr.len);

  memcpy(xr.data, x->data, xr.len*sizeof(pol_item_t));
  memcpy(xl.data, x->data + xr.len, xl.len*sizeof (pol_item_t));
  memcpy(yr.data, y->data, yr.len*sizeof(pol_item_t));
  memcpy(yl.data, y->data + yr.len, yl.len*sizeof (pol_item_t));

  pol_print_descr("xl", xl);
  pol_print_descr("xr", xr);
  pol_print_descr("yl", yl);
  pol_print_descr("yr", yr);

  p1 = pol_karatsuba_mul(&xl, &yl);
  pol_print_descr("p1", p1);

  p2 = pol_karatsuba_mul(&xr, &yr);
  pol_print_descr("p2", p2);
  xlr = pol_sum(xl, xr);
  pol_print_descr("xlr", xlr);
  ylr = pol_sum(yl, yr);
  pol_print_descr("ylr", ylr);
  p3 = pol_karatsuba_mul(&xlr, &ylr);
  pol_print_descr("p3", p3);
  p31 = pol_sub(p3, p1);
  pol_print_descr("p31", p31);
  p312 = pol_sub(p31, p2);
  pol_print_descr("p312", p312);

  pol_normalize_len(&p2);
  pol_normalize_len(&p312);
  pol_normalize_len(&p1);

  pol_pow10(&p1, n);
  pol_pow10(&p312, n - n/2);
  pol_print_descr("p1p", p1);
  pol_print_descr("p312p", p312);

  s1 = pol_sum(p1, p312);
  res = pol_sum(s1, p2);

  pol_free(s1);
  pol_free(p312);
  pol_free(p31);
  pol_free(p3);
  pol_free(ylr);
  pol_free(xlr);
  pol_free(p2);
  pol_free(p1);
  pol_free(yr);
  pol_free(yl);
  pol_free(xr);
  pol_free(xl);

  return res;
}
///////////////////////////////////////////////////////
