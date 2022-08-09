#include "lcs.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define max(x, y) ((x) > (y) ? (x) : (y))

/* Returns length of LCS for X[0..m-1], Y[0..n-1] */
char *lcs(const char *X, const char *Y) {
  int m = strlen(X);
  int n = strlen(Y);
  const int width = n + 1;
  const int height = m + 1;
  // intitalizing a matrix of size (m+1)*(n+1)
  int *L = malloc(sizeof(int) * width * height);
  for (int i = 0; i <= m; i++) {
    for (int j = 0; j <= n; j++) {
      if (i == 0 || j == 0) {
        L[i * width + j] = 0;
      } else if (X[i - 1] == Y[j - 1]) {
        L[i * width + j] = L[(i - 1) * width + (j - 1)] + 1;
      } else {
        L[i * width + j] = max(L[(i - 1) * width + j], L[i * width + j - 1]);
      }
    }
  }
  //

  int idx = L[m * width + n];
  char *lcs_str = malloc(idx + 1); // todo remove VLA
  lcs_str[idx] = '\0';             // Set the terminating character
  int i = m, j = n;
  while (i > 0 && j > 0) {
    // If current character in X[] and Y are same, then
    // current character is part of LCS
    if (X[i - 1] == Y[j - 1]) {
      lcs_str[idx - 1] = X[i - 1]; // Put current character in result
      --i;
      --j;
      --idx;
    } else if (L[(i - 1) * width + j] > L[i * width + (j - 1)]) {
      --i;
    } else {
      --j;
    }
  }

  free(L);
  return lcs_str;
}
//////////////////////////////////////////////////////////////
