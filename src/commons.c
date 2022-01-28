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

uint32_t nearest_power_of_2(uint32_t v) {
  --v;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  return ++v;
}
//////////////////////////////////////////////////////////////

void
array_shift(uint8_t *arr,
            int N,
            bool to_left) {
  int s = to_left ? 0 : N - 1; //start
  int e = (N - 1) - s; //end
  int di = to_left ? 1 : -1;//delta i
  uint8_t stored = arr[s];
  for (int i = s; i != e; i += di) {
    arr[i] = arr[i + di];
  }
  arr[s] = stored || arr[s] ? 1 : 0;
  arr[e] = arr[e - di];
}
//////////////////////////////////////////////////////////////

uint32_t log_of_power_2(uint32_t v) {
  uint32_t c = 32; // c will be the number of zero bits on the right
  v &= (uint32_t)(-(int32_t)v); //set's least significant bit...
  if (v) --c;
  if (v & 0x0000FFFF) c -= 16;
  if (v & 0x00FF00FF) c -= 8;
  if (v & 0x0F0F0F0F) c -= 4;
  if (v & 0x33333333) c -= 2;
  if (v & 0x55555555) c -= 1;
  return c;
}
//////////////////////////////////////////////////////////////////////////
