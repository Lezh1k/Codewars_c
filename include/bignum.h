#ifndef KARATSUBA_H
#define KARATSUBA_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef int32_t pol_limb_t;
typedef int64_t pol_dlimb_t;

// base and base_digits must be consistent
//#define POL_BASE 100000000
//#define POL_BASE_DIGITS 8
//#define POL_LIMB_FORMAT "%08u"

#define POL_BASE 10
#define POL_BASE_DIGITS 1
#define POL_LIMB_FORMAT "%01u"

typedef struct pol {
  pol_limb_t *data;
  uint32_t len;
  int8_t sign;
} pol_t;

typedef enum pol_sign {
  pols_neg = -1,
  pols_pos = 1
} pol_sign_t;

pol_t pol_new(uint32_t len, pol_sign_t sign);
void pol_free(pol_t *p);
pol_t pol_from_str(const char *str);

char *pol_to_str(const pol_t *p);
void pol_print_raw(const pol_t *p);
void pol_print(const pol_t *p, const char *descr);

pol_t pol_inv(const pol_t *p);
pol_t pol_abs(const pol_t *p);

///
/// \brief pol_cmp - compares two polinomes
/// \param l - left one
/// \param r - right one
/// \return res > 0 if l > r, res < 0 if l < r and res == 0 if l == r
///
int pol_cmp(const pol_t *l, const pol_t *r);

pol_t pol_sum(const pol_t *l, const pol_t *r);
pol_t pol_sub(const pol_t *l, const pol_t *r);
pol_t pol_mul(const pol_t *l, const pol_t *r);

void pol_karatsuba_prepare(pol_t *l, pol_t *r);
pol_t pol_mul_karatsuba(pol_t *l, pol_t *r);

#endif // KARATSUBA_H
