#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "trench_assault.h"

typedef union short_str {
  uint32_t val;
  char arr[4];
} short_str_t;

static const char left_side_letters[] = {'s', 'b', 'p', 'w', 0};
static const char right_side_letters[] = {'z', 'd', 'q', 'm', 0};
static const char relief_letters[] = {' ', '-', '|', 0};

static __attribute__((noinline)) int zbyte_32(uint32_t x);
static int weight(char s);
static int weight_asm(char s);
static int weight_slow(char letter);
static void profile_weight_functions(void);
extern int zbyte_32_asm(uint32_t x);

int zbyte_32(uint32_t x) {
  // set 0x8<something> to all bytes except 0.
  // for 0 byte set 0x7f
  uint32_t y = (x & 0x7f7f7f7f) + 0x7f7f7f7f;
  // (y | x | 0x7f7f7f7f) -> set 0xff to all bytes except 0. for 0 - 0x7f
  // inverting gives 0x80 where 0 byte was.
  y = ~(y | x | 0x7f7f7f7f);
  if (y == 0) {
    return -1;
  }
  int n = __builtin_ctz(y);
  return n >> 3;
}

int weight(char s) {
  uint32_t s_msk = (uint32_t)s * 0x01010101;
  uint32_t relief_val = *(const uint32_t *)relief_letters;
  uint32_t left_side_val = *(const uint32_t *)left_side_letters;
  uint32_t right_side_val = *(const uint32_t *)right_side_letters;
  int w = zbyte_32(relief_val ^ s_msk);
  if (w != -1)
    return 0;
  w = zbyte_32(right_side_val ^ s_msk);
  if (w != -1)
    return w + 1;
  w = zbyte_32(left_side_val ^ s_msk);
  if (w != -1)
    return -w - 1;
  __builtin_unreachable();
}
//////////////////////////////////////////////////////////////

int weight_asm(char s) {
  uint32_t s_msk = (uint32_t)s * 0x01010101;
  uint32_t relief_val = *(const uint32_t *)relief_letters;
  uint32_t left_side_val = *(const uint32_t *)left_side_letters;
  uint32_t right_side_val = *(const uint32_t *)right_side_letters;
  int w = zbyte_32_asm(relief_val ^ s_msk);
  if (w != -1)
    return 0;
  w = zbyte_32_asm(right_side_val ^ s_msk);
  if (w != -1)
    return w + 1;
  w = zbyte_32_asm(left_side_val ^ s_msk);
  if (w != -1)
    return -w - 1;
  __builtin_unreachable();
}
//////////////////////////////////////////////////////////////

int weight_slow(char s) {
  for (const char *pl = relief_letters; *pl; ++pl) {
    if (*pl != s)
      continue;
    return 0;
  }

  for (const char *pl = left_side_letters; *pl; ++pl) {
    if (*pl != s)
      continue;
    return -((int)(pl - left_side_letters) + 1);
  }

  for (const char *pl = right_side_letters; *pl; ++pl) {
    if (*pl != s)
      continue;
    return (int)(pl - right_side_letters) + 1;
  }
  // invalid input, raise error
  exit(1);
}
//////////////////////////////////////////////////////////////

extern int find_char_index(const char *str, char target);

static int weight_slow_2(char s) {
  int idx = find_char_index(relief_letters, s);
  if (idx != -1) {
    return 0;
  }

  idx = find_char_index(left_side_letters, s);
  if (idx != -1) {
    return -idx;
  }

  idx = find_char_index(right_side_letters, s);
  if (idx != -1) {
    return idx;
  }
  __builtin_unreachable();
}

static void profile_weight_functions(void) {
  clock_t start, end;
  double cpu_time_used;

  const int iterations = 10000000;
  /* const int iterations = 1; */
  const char *tst = "sbpwzdqm -|sbpwzdqm -|sbpwzdqm -|sbpwzdqm -|";
  /* const char *tst = "sbpwzdqm -|"; */

  printf("slow:\n");
  start = clock();
  for (int i = 0; i < iterations; ++i) {
    for (const char *ps = tst; *ps; ++ps) {
      int w = weight_slow(*ps);
      /* printf("%d ", w); */
    }
    /* printf("\n"); */
  }
  end = clock();
  cpu_time_used =
      ((double)(end - start)) / CLOCKS_PER_SEC; // Calculate time in seconds
  printf("slow()\ttook %.10f seconds to execute\n", cpu_time_used);

  printf("fast:\n");
  start = clock();
  for (int i = 0; i < iterations; ++i) {
    for (const char *ps = tst; *ps; ++ps) {
      int w = weight(*ps);
      /* printf("%d ", w); */
    }
    /* printf("\n"); */
  }
  end = clock();
  cpu_time_used =
      ((double)(end - start)) / CLOCKS_PER_SEC; // Calculate time in seconds
  printf("fast()\ttook %.10f seconds to execute\n", cpu_time_used);

  printf("asm:\n");
  start = clock();
  for (int i = 0; i < iterations; ++i) {
    for (const char *ps = tst; *ps; ++ps) {
      int w = weight_asm(*ps);
      /* printf("%d ", w); */
    }
    /* printf("\n"); */
  }
  end = clock();
  cpu_time_used =
      ((double)(end - start)) / CLOCKS_PER_SEC; // Calculate time in seconds
  printf("asm()\ttook %.10f seconds to execute\n", cpu_time_used);

  printf("slow 2:\n");
  start = clock();
  for (int i = 0; i < iterations; ++i) {
    for (const char *ps = tst; *ps; ++ps) {
      int w = weight_slow_2(*ps);
      /* printf("%d ", w); */
    }
    /* printf("\n"); */
  }
  end = clock();
  cpu_time_used =
      ((double)(end - start)) / CLOCKS_PER_SEC; // Calculate time in seconds
  printf("slow2()\ttook %.10f seconds to execute\n", cpu_time_used);
}
//////////////////////////////////////////////////////////////

int trench_assault_main(int argc, const char *argv[]) {
  (void)argc;
  (void)argv;
  profile_weight_functions();
  return 0;
}
