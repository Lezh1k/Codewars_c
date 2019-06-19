#include <stdio.h>
#include <stdlib.h>
#include <nmmintrin.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <ctype.h>
#include "bignum.h"

typedef struct pol_pairc {
  const pol_t *big;
  const pol_t *small;
} pol_pairc_t;

static uint32_t str2ui(const char *str, uint32_t len);
static uint32_t powi(uint32_t base, uint32_t exp);
static pol_pairc_t pol_sum_pairc(const pol_t *l, const pol_t *r);
static void pol_increase_len(pol_t *p, uint32_t l);
static pol_t pol_part(const pol_t *src, uint32_t start, uint32_t len);

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

  rbuff = calloc(1, p->len * POL_BASE_DIGITS + 2);
  result = rbuff;
  if (p->sign == pols_neg)
    *rbuff++ = '-';
  *rbuff = '0';

  pol_limb_t *ptr = (pol_limb_t*) &p->data[p->len-1];

  //don't print front zeros
  while (ptr >= p->data && *ptr == 0)
    --ptr;

  if (ptr < p->data)
    return result;

  int offset = sprintf(rbuff, "%u", *ptr--);
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
  pol_limb_t carry = 0;
  for (uint32_t i=0; i < l->len; ++i) {
    for (uint32_t j=0; j < r->len || carry; ++j) {
      pol_dlimb_t cur = res.data[i+j] + (pol_dlimb_t)l->data[i] * (j < r->len ? r->data[j] : 0) + carry;
      res.data[i+j] = (pol_limb_t)(cur % POL_BASE);
      carry = (pol_limb_t) (cur / POL_BASE);
    }
  }

  pol_trim(&res);
  return res;
}
///////////////////////////////////////////////////////

static uint32_t nearest2pow(uint32_t v) {
  --v;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  return ++v;
}
//////////////////////////////////////////////////////////////////////////

void pol_increase_len(pol_t *p, uint32_t l) {
  assert(p->len <= l);
  if (p->len == l) return;
  pol_limb_t *tmp = malloc(l * sizeof(pol_limb_t));
  memcpy(tmp, p->data, p->len*sizeof(pol_limb_t));
  memset(&tmp[p->len], 0, (l - p->len)*sizeof(pol_limb_t));
  free(p->data);
  p->data = tmp;
  p->len = l;
}
///////////////////////////////////////////////////////

void pol_karatsuba_prepare(pol_t *l, pol_t *r) {
  uint32_t ml = l->len > r->len ? l->len : r->len;
  ml = nearest2pow(ml);
  if (l->len != ml)
    pol_increase_len(l, ml);
  if (r->len != ml)
    pol_increase_len(r, ml);
}
///////////////////////////////////////////////////////

pol_t pol_part(const pol_t *src, uint32_t start, uint32_t len) {
  pol_t res = pol_new(len, src->sign);
  memcpy(res.data, &src->data[start], len * sizeof (pol_limb_t));
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

static pol_t pol_mul_karatsuba_int(pol_t *l, pol_t *r) {
  uint32_t n = l->len;
  if (n == 1) {
    pol_t res = pol_new(2, l->sign * r->sign);
    pol_dlimb_t cur = (pol_dlimb_t)l->data[0] * (pol_dlimb_t)(r->data[0]);
    res.data[0] = (pol_limb_t)(cur % POL_BASE);
    res.data[1] = (pol_limb_t)(cur / POL_BASE);
    pol_trim(&res);
    return res;
  }

  pol_t res = pol_new(n+n, l->sign*r->sign);
  uint32_t k = n >> 1;

  pol_t xr = pol_part(l, 0, k);
  pol_t xl = pol_part(l, k, k);
  pol_t yr = pol_part(r, 0, k);
  pol_t yl = pol_part(r, k, k);

  pol_t p1 = pol_mul_karatsuba_int(&xl, &yl);
  pol_t p2 = pol_mul_karatsuba_int(&xr, &yr);

  for (uint32_t i = 0; i < k; ++i) {
    xl.data[i] += xr.data[i];
    yl.data[i] += yr.data[i];
  }

  pol_t p3 = pol_mul_karatsuba_int(&xl, &yl);

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


  //normalization of negative values
  pol_limb_t carry = 0;
  for (uint32_t i = 0; i < res.len; ++i) {
    res.data[i] = res.data[i] - carry;
    carry = 0;
    while (res.data[i] < 0) {
      ++carry;
      res.data[i] += POL_BASE;
    }
  }

  pol_free(&xl);
  pol_free(&xr);
  pol_free(&yl);
  pol_free(&yr);
  pol_free(&p1);
  pol_free(&p2);
  pol_free(&p3);
  return res;
}
///////////////////////////////////////////////////////

pol_t pol_mul_karatsuba(pol_t *l, pol_t *r) {
  pol_karatsuba_prepare(l, r);
  pol_t tmp = pol_mul_karatsuba_int(l, r);
  pol_t res = pol_new(l->len + r->len, l->sign * r->sign);
  pol_limb_t carry = 0;
  for (uint32_t i = 0; i < tmp.len; ++i) {
    pol_dlimb_t cur = (pol_dlimb_t) (tmp.data[i] + carry);
    res.data[i] = cur % POL_BASE;
    carry = (pol_limb_t) (cur / POL_BASE);
  }
  pol_trim(&res);
  pol_free(&tmp);
  return res;
}
///////////////////////////////////////////////////////

void pol_print_raw(const pol_t *p) {
  for (uint32_t i = 0; i < p->len; ++i)
    printf("%d ", p->data[i]);
  printf("\n");
}
///////////////////////////////////////////////////////

void pol_print(const pol_t *p, const char *descr) {
  char *s = pol_to_str(p);
  printf("%s: %s\n", descr, s);
  free(s);
}
///////////////////////////////////////////////////////
