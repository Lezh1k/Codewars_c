#include <stdbool.h>

/**
 * A B C
 * D E F
 * G H I
 * */

static const char *direct_connections[] = {
    "BDEFH",    // a
    "ACDEFGI",  // b
    "BDEFH",    // c
    "ABCEGHI",  // d
    "ABCDFGHI", // e
    "ABCEGHI",  // f
    "BDEFH",    // g
    "ACDEFGI",  // h
    "BDEFH",    // i
};

static const char *indirect_connections[] = {
    "BCDGEI", // a
    "EH",     // b
    "BAEGFI", // c
    "EF",     // d
    "",       // e
    "ED",     // f
    "DAECHI", // g
    "EB",     // h
    "EAFCHG", // i
};

typedef struct dot {
  bool is_visited;
} dot_t;

dot_t dot(void) {
  dot_t res = {.is_visited = false};
  return res;
}

static int c2idx(char c) { return (int)(c - 'A'); }
//////////////////////////////////////////////////////////////

static int count_patterns_rec(int root_idx, int left, dot_t field[9]) {
  if (left == 0) {
    return 1;
  }

  int sum = 0;
  field[root_idx].is_visited = true;
  for (const char *pc = direct_connections[root_idx]; *pc; ++pc) {
    int n0 = c2idx(*pc);
    if (field[n0].is_visited) {
      continue;
    }
    sum += count_patterns_rec(n0, left - 1, field);
  }

  for (const char *ic = indirect_connections[root_idx]; *ic; ic += 2) {
    int n1 = c2idx(ic[0]);
    if (!field[n1].is_visited) {
      continue;
    }
    int n2 = c2idx(ic[1]);
    if (field[n2].is_visited) {
      continue;
    }
    sum += count_patterns_rec(n2, left - 1, field);
  }

  field[root_idx].is_visited = false;
  return sum;
}

unsigned count_patterns(char start, unsigned pattern_length) {
  if (pattern_length == 0) {
    return 0;
  }
  dot_t field[9] = {0};
  return count_patterns_rec(c2idx(start), pattern_length - 1, field);
}
//////////////////////////////////////////////////////////////
