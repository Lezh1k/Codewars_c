#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "skyscapers.h"

#define SIZE 6

int check(int **solution, int (*expected)[SIZE]) {
  bool result = false;
  if (solution && expected) {
    result = true;
    for (int i = 0; i < SIZE; i++)
      if (memcmp(solution[i], expected[i], SIZE * sizeof(int))) {
        return false;
      }
  }
  return result;
}
///////////////////////////////////////////////////////

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  setbuf(stdout, NULL);

  static const int clues[][SIZE*4] = {
    {
      3, 2, 2, 3, 2, 1,
      1, 2, 3, 3, 2, 2,
      5, 1, 2, 2, 4, 3,
      3, 2, 1, 2, 2, 4
    },
    {
      0, 0, 0, 2, 2, 0,
      0, 0, 0, 6, 3, 0,
      0, 4, 0, 0, 0, 0,
      4, 4, 0, 3, 0, 0
    }
  };

  static int expected[][SIZE][SIZE] = {
    { { 2, 1, 4, 3, 5, 6 },
      { 1, 6, 3, 2, 4, 5 },
      { 4, 3, 6, 5, 1, 2 },
      { 6, 5, 2, 1, 3, 4 },
      { 5, 4, 1, 6, 2, 3 },
      { 3, 2, 5, 4, 6, 1 } },
    { { 5, 6, 1, 4, 3, 2 },
      { 4, 1, 3, 2, 6, 5 },
      { 2, 3, 6, 1, 5, 4 },
      { 6, 5, 4, 3, 2, 1 },
      { 1, 2, 5, 6, 4, 3 },
      { 3, 4, 2, 5, 1, 6 } }
  };

  if (!check(SolvePuzzle(clues[0]), expected[0]))
    return -1;
  if (!check(SolvePuzzle(clues[1]), expected[1]))
    return -2;

  return 0;
}
/////////////////////////////////////////////////////////
