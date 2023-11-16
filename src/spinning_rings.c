#include "spinning_rings.h"
typedef unsigned long long ull;

ull spinning_rings(ull inner_max, ull outer_max) {
  ull i, o, c;
  i = o = c = 0;
  ++inner_max;
  ++outer_max;

  i = (i + inner_max - 1) % (inner_max);
  o = (o + 1) % (outer_max);
  ++c;

  while (i != o) {
    ull k = 1;
    // check edge cases
    if (o >= inner_max) {
      k = outer_max - o; // we need to scroll until 0
    } else if (i > outer_max) {
      k = i - outer_max; // we need to scroll i to outer_max value
    } else if (i > o) {
      k = ((i - o) + 1) / 2;
    } else if (o > i) {
      // what's closer - o to outer_max or i to 0?
      ull o2om = outer_max - o;
      ull i2z = i + 1;
      k = o2om < i2z ? o2om : i2z; // get min step
    }

    i = (i + inner_max - (k % inner_max)) % inner_max;
    o = (o + k) % (outer_max);
    c += k;
  }

  return c;
}

void dotest(ull im, ull om, ull exp) {
  ull act = spinning_rings(im, om);
  printf("%lld == %lld, %s\n", exp, act, exp == act ? "true" : "false");
}
//////////////////////////////////////////////////////////////
