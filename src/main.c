#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

static int row_start_idx(int idx) { return (idx * (idx + 1)) / 2; }
static int element_idx(int row, int col) { return row_start_idx(row) + col; }

// Memory model of `pyramid`:
// `pyramid` is a one-dimensional array containing the elements of a pyramid
// with `rows` levels, in direct succession from left to right, top to bottom
static int pyramid_slide_down(int num_elements, int pyramid[num_elements],
                              int rows) {
  for (int r = rows - 2; r >= 0; --r) {
    for (int c = 0; c < r + 1; ++c) {
      int dst = element_idx(r, c);
      int li = element_idx(r + 1, c);
      int ri = element_idx(r + 1, c + 1);
      pyramid[dst] += pyramid[li] > pyramid[ri] ? pyramid[li] : pyramid[ri];
    }
  }
  return pyramid[0];
}
//////////////////////////////////////////////////////////////

int main(int argc, const char *argv[]) {
  (void)argc;
  (void)argv;
  // clang-format off
  int input[] = {3, 
                 7, 2,
                 1, 4, 6,
                 8, 1, 9, 3,
                 9, 9, 9, 9, 9};
  // clang-format on
  int expected = 32;
  int actual = pyramid_slide_down(15, input, 5);

  printf("%d==%d\n", actual, expected);
  return 0;
}
//////////////////////////////////////////////////////////////
