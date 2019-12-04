#ifndef MERGE_INTERVALS_H
#define MERGE_INTERVALS_H
#include <stdlib.h>

typedef struct interval {
  int first;
  int second;
} interval_t;

int sum_intervals(const interval_t *v, size_t n);

#endif // MERGE_INTERVALS_H
