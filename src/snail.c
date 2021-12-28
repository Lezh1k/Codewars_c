#include "snail.h"
#include <stddef.h>
#include <errno.h>
#include <stdio.h>

int*
snail(size_t *outsz,
      const int **mx,
      size_t m,
      size_t n) {

  if (m == 0 || n == 0) {
    *outsz = 0;
    return NULL;
  }

  *outsz = m*n;
  int *res = malloc(*outsz * sizeof(int));
  int cx, cy;
  cy = cx = 0;

  for (size_t c = 0; c < *outsz;) {
    int l = cx;
    int u = cy;

    //go right
    for (; cx < m; ++cx) {
      res[c++] = mx[cy][cx];
    }
    --cx; ++cy;

    //go down
    for (; cy < n; ++cy) {
      res[c++] = mx[cy][cx];
    }
    --cy; --cx;

    //go left
    for (; cx >= l; --cx) {
      res[c++] = mx[cy][cx];
    }
    ++cx; --cy;

    //go up
    for (; cy > u; --cy) {
      res[c++] = mx[cy][cx];
    }
    ++cy; ++cx;
    --n; --m;
  }

  return res;
}
