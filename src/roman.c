#include <stdio.h>
#include <assert.h>
#include "roman.h"

static int roman2arabic(char c) {
  switch(c) {
    case 'I' : return 1;
    case 'V' : return 5;
    case 'X' : return 10;
    case 'L' : return 50;
    case 'C' : return 100;
    case 'D' : return 500;
    case 'M' : return 1000;
    default:
      assert(0);
      return -1;
  }
}
///////////////////////////////////////////////////////

int from_roman(char *r) {
  int ld = 1000;
  int ar = 0;
  int d;
  for (; *r; ++r) {
    d = roman2arabic(*r);
    if (ld < d)
      ar -= 2 * ld;
    ld = d;
    ar += ld;
  }
  return ar;
}
///////////////////////////////////////////////////////

void to_roman(int src, char *dst) {
  enum { count = 13 };
  static const int ints[count] = {1000, 900, 500, 400, 100, 90, 50, 40, 10, 9, 5, 4, 1};
  static const char *roma[count] = {"M", "CM", "D", "CD", "C", "XC", "L", "XL", "X", "IX", "V", "IV", "I"};
  assert(src >= 0 && src < 4000);
  int key;
  int q;

  for (key = 0; key < count && src; ++key) {
    if (src < ints[key])
      continue;
    q = src / ints[key];
    while (q--) {
      dst += sprintf(dst, "%s", roma[key]);
      src -= ints[key];
    }
  }
  *dst = 0;
}
///////////////////////////////////////////////////////
