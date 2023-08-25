#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parse_int.h"

static long natural_str_to_long(const char *str, int slen);
static int cmp_nlp_num_pair_by_str_asc(const void *l_, const void *r_);

typedef struct nlp_num_pair {
  const char *str;
  size_t len;
  long num;
} nlp_num_pair_t;
//////////////////////////////////////////////////////////////

int cmp_nlp_num_pair_by_str_asc(const void *l_, const void *r_) {
  const nlp_num_pair_t *l = (const nlp_num_pair_t *)l_;
  const nlp_num_pair_t *r = (const nlp_num_pair_t *)r_;
  if (l->len == r->len)
    return strncmp(l->str, r->str, l->len);
  return l->len - r->len;
}
//////////////////////////////////////////////////////////////

static void _process_num(long *res, long *st, long num) {
  if (num == 100) {
    *st *= 100;
  } else if (num == 1000 || num == 1000000) {
    *st *= num;
    *res += *st;
    *st = 0;
  } else {
    *st += num;
  }
}
//////////////////////////////////////////////////////////////

long parse_int(const char *str) {
  char *p_sd = NULL;
  long res = 0;
  long st = 0;

  for (p_sd = strpbrk(str, " -"); p_sd; p_sd = strpbrk(str, " -")) {
    int slen = p_sd - str;
    long num = natural_str_to_long(str, slen);
    if (num != -1)
      _process_num(&res, &st, num);
    printf("%.*s => ", slen, str);
    printf("res: %ld, st: %ld, num: %ld\n", res, st, num);
    str = p_sd + 1;
  }

  int slen = strlen(str);
  long num = natural_str_to_long(str, slen);
  if (num != -1)
    _process_num(&res, &st, num);

  res += st;
  printf("res: %ld, st: %ld, num: %ld\n", res, st, num);
  return res;
}
//////////////////////////////////////////////////////////////

long natural_str_to_long(const char *str, int slen) {
  // sorted by str
  const nlp_num_pair_t pairs[] = {
      {.str = "one", .len = 3, .num = 1},
      {.str = "six", .len = 3, .num = 6},
      {.str = "ten", .len = 3, .num = 10},
      {.str = "two", .len = 3, .num = 2},
      {.str = "five", .len = 4, .num = 5},
      {.str = "four", .len = 4, .num = 4},
      {.str = "nine", .len = 4, .num = 9},
      {.str = "zero", .len = 4, .num = 0},
      {.str = "eight", .len = 5, .num = 8},
      {.str = "fifty", .len = 5, .num = 50},
      {.str = "forty", .len = 5, .num = 40},
      {.str = "seven", .len = 5, .num = 7},
      {.str = "sixty", .len = 5, .num = 60},
      {.str = "three", .len = 5, .num = 3},
      {.str = "eighty", .len = 6, .num = 80},
      {.str = "eleven", .len = 6, .num = 11},
      {.str = "ninety", .len = 6, .num = 90},
      {.str = "thirty", .len = 6, .num = 30},
      {.str = "twelve", .len = 6, .num = 12},
      {.str = "twenty", .len = 6, .num = 20},
      {.str = "fifteen", .len = 7, .num = 15},
      {.str = "hundred", .len = 7, .num = 100},
      {.str = "million", .len = 7, .num = 1000000},
      {.str = "seventy", .len = 7, .num = 70},
      {.str = "sixteen", .len = 7, .num = 16},
      {.str = "eighteen", .len = 8, .num = 18},
      {.str = "fourteen", .len = 8, .num = 14},
      {.str = "nineteen", .len = 8, .num = 19},
      {.str = "thirteen", .len = 8, .num = 13},
      {.str = "thousand", .len = 8, .num = 1000},
      {.str = "seventeen", .len = 9, .num = 17},
  };

  const nlp_num_pair_t dummy_pair = {.str = str, .len = slen, .num = -1};
  nlp_num_pair_t *ptr_found =
      bsearch((const void *)&dummy_pair, (const void *)pairs,
              sizeof(pairs) / sizeof(nlp_num_pair_t), sizeof(nlp_num_pair_t),
              cmp_nlp_num_pair_by_str_asc);

  if (ptr_found == NULL) {
    fprintf(stderr, "couldn't find '%.*s' in known numbers dictionary\n", slen,
            str);
    return -1; // shit happened
  }
  return ptr_found->num;
}
//////////////////////////////////////////////////////////////
