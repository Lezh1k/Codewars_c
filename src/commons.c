#include "commons.h"
#include <stdio.h>
#include <inttypes.h>

int32_t trailing_zeros_32_bi(uint32_t v) {
  return __builtin_ctz(v);
}
///////////////////////////////////////////////////////

int32_t trailing_zeros_64(uint64_t v) {
  int32_t c = 64; // c will be the number of zero bits on the right
  v &= (uint64_t)(-((int64_t)v)); //The expression (v & -v) extracts the least significant 1 bit from v
  if (v) c--;
  if (v & 0x00000000ffffffff) c -= 32;
  if (v & 0x0000ffff) c -= 16;
  if (v & 0x00ff) c -= 8;
  if (v & 0x0f) c -= 4;
  if (v & 0x03) c -= 2;
  if (v & 0x01) c -= 1;
  return c;
}
///////////////////////////////////////////////////////

int32_t leading_zeros_64(uint64_t v) {
  if (v >> 32)
    return 32 + leading_zeros_32(v >> 32);
  return leading_zeros_32((uint32_t)v);
}
///////////////////////////////////////////////////////

int32_t leading_zeros_32(uint32_t v) {
  static const int8_t log_table_256[256] =
  {
  #define LT(n) n, n, n, n, n, n, n, n, n, n, n, n, n, n, n, n
    -1,
    0,
    1, 1,
    2, 2, 2, 2,
    3, 3, 3, 3, 3, 3, 3, 3,
    LT(4),
    LT(5), LT(5),
    LT(6), LT(6), LT(6), LT(6),
    LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7)
  };
  int32_t r;     // r will be lg(v)
  uint32_t t, tt; // temporaries
  if ((tt = v >> 16)) {
    r = (t = tt >> 8) ? 24 + log_table_256[t] : 16 + log_table_256[tt];
  } else {
    r = (t = v >> 8) ? 8 + log_table_256[t] : log_table_256[v];
  }
  return r;
}
///////////////////////////////////////////////////////

int32_t leading_zeros_32_bi(uint32_t v) {
  return __builtin_clz(v);
}
///////////////////////////////////////////////////////
