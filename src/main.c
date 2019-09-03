#include <stdint.h>
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "skyscrapers.h"

#define  SIZE  7

static int clues[][SIZE * 4] = {
  { 7, 0, 0, 0, 2, 2, 3,
    0, 0, 3, 0, 0, 0, 0,
    3, 0, 3, 0, 0, 5, 0,
    0, 0, 0, 0, 5, 0, 4 },
  { 0, 2, 3, 0, 2, 0, 0,
    5, 0, 4, 5, 0, 4, 0,
    0, 4, 2, 0, 0, 0, 6,
    5, 2, 2, 2, 2, 4, 1 },
  { 6, 4, 0, 2, 0, 0, 3,
    0, 3, 3, 3, 0, 0, 4,
    0, 5, 0, 5, 0, 2, 0,
    0, 0, 0, 4, 0, 0, 3 }
};

static int expected[][SIZE][SIZE] = {
  { { 1, 5, 6, 7, 4, 3, 2 },
    { 2, 7, 4, 5, 3, 1, 6 },
    { 3, 4, 5, 6, 7, 2, 1 },
    { 4, 6, 3, 1, 2, 7, 5 },
    { 5, 3, 1, 2, 6, 4, 7 },
    { 6, 2, 7, 3, 1, 5, 4 },
    { 7, 1, 2, 4, 5, 6, 3 } },

  { { 7, 6, 2, 1, 5, 4, 3 },
    { 1, 3, 5, 4, 2, 7, 6 },
    { 6, 5, 4, 7, 3, 2, 1 },
    { 5, 1, 7, 6, 4, 3, 2 },
    { 4, 2, 1, 3, 7, 6, 5 },
    { 3, 7, 6, 2, 1, 5, 4 },
    { 2, 4, 3, 5, 6, 1, 7 } },
};
///////////////////////////////////////////////////////

int check(int **solution, int (*expected)[SIZE]) {
  int result = 0;
  if (solution && expected) {
    result = 1;
    for (int i = 0; i < SIZE; i++)
      if (memcmp(solution[i], expected[i], SIZE * sizeof(int))) {
        result = 0;
        break;
      }
  }
  return result;
}
///////////////////////////////////////////////////////

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
//  setbuf(stdout, NULL);
  SolvePuzzle(clues[2]);  
}
