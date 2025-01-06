#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "trench_assault.h"

typedef enum unit_side {
  US_LEFT = -1,
  US_RIGHT = 1,
  US_RELIEF = 0
} unit_side_t;

typedef struct unit {
  int32_t power;
  int32_t extra_def_power;
  bool can_continue_attack;
  bool is_relief;
} unit_t;

static const char left_side_letters[] = {'s', 'b', 'p', 'w', 0};
static const char right_side_letters[] = {'z', 'd', 'q', 'm', 0};
static const char relief_letters[] = {' ', '-', '|', 0};

static int weight_unrolled(char s);
static int zbyte_32(uint32_t x);
static int weight(char s);
static int weight_slow(char letter);
static void profile_weight_functions(void);

int weight_unrolled(char s) {
  switch (s) {
  case 's':
    return -1;
  case 'b':
    return -2;
  case 'p':
    return -3;
  case 'w':
    return -4;
  case 'z':
    return 1;
  case 'd':
    return 2;
  case 'q':
    return 3;
  case 'm':
    return 4;
  case ' ':
  case '-':
  case '|':
    return 0;
  }
  exit(1);
}

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
  printf("%x\n%x\n%x\n", relief_val, left_side_val, right_side_val);
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

static void profile_weight_functions(void) {
  clock_t start, end;
  double cpu_time_used;

  const int iterations = 10000000;
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
  printf("slow() took %.10f seconds to execute\n", cpu_time_used);

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
  printf("fast() took %.10f seconds to execute\n", cpu_time_used);

  printf("unrolled:\n");
  start = clock();
  for (int i = 0; i < iterations; ++i) {
    for (const char *ps = tst; *ps; ++ps) {
      int w = weight_unrolled(*ps);
      /* printf("%d ", w); */
    }
    /* printf("\n"); */
  }
  end = clock();
  cpu_time_used =
      ((double)(end - start)) / CLOCKS_PER_SEC; // Calculate time in seconds
  printf("unrolled() took %.10f seconds to execute\n", cpu_time_used);
}
//////////////////////////////////////////////////////////////

char *trench_assault(const char *battlefield_in) {
  char *battlefield = strdup(battlefield_in);
  const char *l1 = strtok(battlefield, "\n");
  const char *l2 = strtok(NULL, "\n");

  size_t l1_len = strlen(l1);
  size_t l2_len = strlen(l2);
  assert(l1_len == l2_len);

  unit_t *units = calloc(l1_len + 1, sizeof(unit_t));
  char *res_str = calloc(l1_len + 1, sizeof(char));

  // init
  for (size_t i = 0; i < l1_len; ++i) {
    char H = l1[i];
    char L = l2[i];
    char UC = L == '-' ? H : L;
    units[i].power = weight(UC);
    units[i].is_relief = units[i].power == 0;
    units[i].can_continue_attack = true;
    if (UC == L) {
      if (l2[i - 1] == '|') {
        units[i].extra_def_power = units[i].power;
      }
      if (l2[i + 1] == '|') {
        units[i].can_continue_attack = false;
      }
    }
    printf("%c%d:%d\t", UC, units[i].power, units[i].extra_def_power);
  }
  printf("\n");

  // action
  for (size_t i = 0; i < l1_len; ++i) {
    if (!units[i].can_continue_attack) {
      printf("%d%d%d\t", units[i].power, units[i + 1].extra_def_power,
             units[i + 1].power);
      continue; // do nothing
    }

    int sum = units[i].power;
    if (units[i].power * units[i + 1].power > 0) {
      // same side
      sum += units[i + 1].power;
    } else {
      // opposite sides
      if (abs(units[i].power) > abs(units[i + 1].extra_def_power)) {
        sum += units[i + 1].extra_def_power + units[i + 1].power;
      } else {
        sum = units[i + 1].power;
      }
    }

    printf("%d%d%d%d\t", units[i].power, units[i + 1].extra_def_power,
           units[i + 1].power, sum);
    units[i + 1].power = sum;
  }
  printf("\n");

  for (size_t i = 0; i < l1_len; ++i) {
    res_str[i] = '|';
    if (units[i].is_relief) {
      continue;
    }
    if (units[i].power == 0) {
      res_str[i] = units[i - 1].power < 0 ? 'L' : 'R';
      continue;
    }
    res_str[i] = units[i].power < 0 ? 'L' : 'R';
  }

  free(battlefield);
  printf("%s\n", res_str);
  return res_str;
}
//////////////////////////////////////////////////////////////

int trench_assault_main(int argc, const char *argv[]) {
  (void)argc;
  (void)argv;
  /* const char *battlefield = "pbbzmq\n" */
  /*                           "------"; */
  /* const char *battlefield = "pwss     s\n" */
  /*                           "----|qdd|-"; */
  /* const char *battlefield = "       ww      mmqpsw      \n" */
  /*                           "pszwbw|--|swzd|------|qpbwb"; */
  /* const char *battlefield = "wpzm       \n" */
  /*                           "----|qwzbdm"; */
  /* trench_assault(battlefield); */
  weight(' ');
  /* profile_weight_functions(); */
  return 0;
}
