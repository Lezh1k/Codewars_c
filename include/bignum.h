#ifndef KARATSUBA_H
#define KARATSUBA_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef int32_t bn_word_t;
typedef int64_t bn_dword_t;

// base and base_digits must be consistent
#define BN_BASE 100000000
#define BN_BASE_DIGITS 8
#define BN_WORD_FORMAT "%08u"

//#define BN_BASE 10
//#define BN_BASE_DIGITS 1
//#define BN_WORD_FORMAT "%01u"

typedef struct bn {
  bn_word_t *data;
  uint32_t len;
  int8_t sign;
} bn_t;

typedef enum bn_sign {
  bns_neg = -1,
  bns_pos = 1
} bn_sign_t;

bn_t bn_new(uint32_t len, bn_sign_t sign);
void bn_free(bn_t *p);
bn_t bn_from_str(const char *str);

char *bn_to_str(const bn_t *p);
void bn_print_raw(const bn_t *p);
void bn_print(const bn_t *p, const char *descr);

bn_t bn_inv(const bn_t *p);
bn_t bn_abs(const bn_t *p);

///
/// \brief bn_cmp - compares two polinomes
/// \param l - left one
/// \param r - right one
/// \return res > 0 if l > r, res < 0 if l < r and res == 0 if l == r
///
int bn_cmp(const bn_t *l, const bn_t *r);

bn_t bn_sum(const bn_t *l, const bn_t *r);
bn_t bn_sub(const bn_t *l, const bn_t *r);

bn_t bn_mul_naive(const bn_t *l, const bn_t *r);
bn_t bn_mul_karatsuba(bn_t *l, bn_t *r);

#endif // KARATSUBA_H
