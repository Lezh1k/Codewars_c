#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "spiral.h"

static inline bool is_out_of_bounds(int n, int cr, int cc) {
  return cr < 0 || cr >= n || cc < 0 || cc >= n;
}

void spiralize(int n, int spiral[n][n]) {
  if (n < 5) return ; //do nothing
  for (int i = 0; i < n; ++i)
    memset(spiral[i], 0, sizeof(int)*n);

  static const int dr[] = {0, 1, 0, -1};
  static const int dc[] = {1, 0, -1, 0};
  int cr, cc, cd, tn;
  cr = cc = cd = tn = 0;

#define change_dir() do { \
    cd = (cd + 1) % 4; ++tn; \
  } while(0);

#define set_cell() do { \
    spiral[cr][cc] = 1; tn = 0; \
  } while(0);

  while(tn < 2) {
    int next_r = cr + dr[cd];
    int next_c = cc + dc[cd];
    int next_r2 = next_r + dr[cd];
    int next_c2 = next_c + dc[cd];

    // check next cell.
    if (is_out_of_bounds(n, next_r, next_c)) {
      change_dir();
      continue;
    }

    if (!is_out_of_bounds(n, next_r2, next_c2)) {
      if (spiral[next_r2][next_c2]) {
        change_dir();
        continue;
      }
    }

    // check neighbours number
    int nn = 0;
    for (int dir_ix = 0; dir_ix < 4; ++dir_ix) {
      int nr = cr + dr[dir_ix];
      int nc = cc + dc[dir_ix];
      if (is_out_of_bounds(n, nr, nc))
        continue;
      nn += spiral[nr][nc];
    }
    if (nn > 1) {
      break;
    }

    set_cell();

    cr = next_r;
    cc = next_c;
  }

  if (tn == 2) { spiral[cr][cc] = 1; }
}
//////////////////////////////////////////////////////////////

void
print_spiral(int n, const int spiral[n][n]) {
  for (int r = 0; r < n; ++r) {
    for (int c = 0; c < n; ++c) {
      printf("%d\t", spiral[r][c]);
    }
    printf("\n");
  }
}
