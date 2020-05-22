#ifndef SNAIL_H
#define SNAIL_H

#include <stdlib.h>
#include <stdio.h>

int **from_static_arr(int *sa, int N) {
  int **res = malloc(N * sizeof(int*));
  for (int i = 0; i < N; ++i)
    res[i] = malloc(N * sizeof(int));

  for (int y = 0; y < N; ++y)
    for (int x = 0; x < N; ++x)
      res[y][x] = *sa++;
  return res;
}
///////////////////////////////////////////////////////

void print_snailed(int *sn, int size){
  printf("\n******************\n");
  for (int i = 0; i < size; ++i) {
    printf("%d ", sn[i]);
  }
  printf("\n");
}
///////////////////////////////////////////////////////

/*
  size_t outsz;
  int srcData4[] = {
    1, 2, 3, 4,
    12, 13, 14, 5,
    11, 16, 15, 6,
    10, 9, 8, 7
  };
  int **data4 = from_static_arr(srcData4, 4);
  int *sn4 = snail(&outsz, data4, 4, 4);
  print_snailed(sn4, outsz);

  int srcData5[] = {
    1,  2,  3,  4,  5,
    16, 17, 18, 19, 6,
    15, 24, 25, 20, 7,
    14, 23, 22, 21, 8,
    13, 12, 11, 10, 9,
  };

  int **data5 = from_static_arr(srcData5, 5);
  int *sn5 = snail(&outsz, data5, 5, 5);
  print_snailed(sn5, outsz);
*/

int *
snail(size_t *outsz,
      const int **mx,
      size_t m,
      size_t n);

#endif // SNAIL_H
