#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "skyscapers.h"

/*
#define  SIZE  6

static int clues[][SIZE * 4] = {
  { 3, 2, 2, 3, 2, 1,
    1, 2, 3, 3, 2, 2,
    5, 1, 2, 2, 4, 3,
    3, 2, 1, 2, 2, 4 },
  { 0, 0, 0, 2, 2, 0,
    0, 0, 0, 6, 3, 0,
    0, 4, 0, 0, 0, 0,
    4, 4, 0, 3, 0, 0 },
  { 4, 4, 0, 3, 0, 0,
    0, 0, 0, 2, 2, 0,
    0, 0, 0, 6, 3, 0,
    0, 4, 0, 0, 0, 0 },
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

int check(int **solution, int (*expected)[SIZE]) {
  int result = 0;
  if (solution && expected) {
    result = 1;
    for (int i = 0; i < SIZE; i++) {
      if (memcmp(solution[i], expected[i], SIZE * sizeof(int))) {
        result = 0;
        break;
      }
    }
  }

  return result;
}
///////////////////////////////////////////////////////*/

#define SIZE 6
#define FULL 0x3f

typedef struct state {
  //main
  int32_t m_mtx[SIZE][SIZE];
  int32_t m_clues[SIZE*4];

  //aux
  uint8_t m_col_mask[SIZE];
  uint8_t m_row_mask[SIZE];

  //in this order we should solve
  int32_t m_row_indexes[SIZE];
  int32_t m_col_indexes[SIZE];
} state_t;
///////////////////////////////////////////////////////

typedef struct line {
  int32_t ix;
  int32_t clN;
} line_t;

static int line_cmp_desc(const void* l1, const void* l2) {
  const line_t *pl2, *pl1;
  pl2 = (const line_t*)l2;
  pl1 = (const line_t*)l1;

  if (pl2->clN == pl1->clN)
    return pl1->ix - pl2->ix;
  return pl2->clN - pl1->clN;
}
///////////////////////////////////////////////////////

typedef enum direction {
  up=0,
  down,
  left,
  right
} direction_t;
///////////////////////////////////////////////////////

static state_t new_state(void);
static void init_clues(state_t *st, const int *clues);

static int row_n(const state_t *st, int32_t r, direction_t d);
static int col_n(const state_t *st, int32_t c, direction_t d);
static int row_n2(const state_t *st, int32_t r, direction_t d);
static int col_n2(const state_t *st, int32_t c, direction_t d);

static bool set_item(state_t *st, int32_t r, int32_t c, int32_t it);
static void clear_item(state_t *st, int32_t r, int32_t c);

static bool state_is_final(const state_t *st);
static bool check_clues(const state_t *st, int32_t r, int32_t c);

static bool row_is_ok(const state_t *st, int32_t r);
static bool col_is_ok(const state_t *st, int32_t c);

static void solve_edge_row(state_t *st, int32_t r);
static void solve_edge_col(state_t *st, int32_t c);

static void solve_edge(state_t* st);

static bool solve_cleared(state_t *st, int32_t ri, int32_t ci, int32_t it);

static void print_state(const state_t *st);

state_t new_state() {
  int32_t r;
  state_t st;
  for (r = 0; r < SIZE; ++r)
    memset(st.m_mtx[r], 0, sizeof(int32_t)*SIZE);
  memset(st.m_col_mask, 0, sizeof (int32_t)*SIZE);
  memset(st.m_row_mask, 0, sizeof (int32_t)*SIZE);
  return st;
}
///////////////////////////////////////////////////////

void init_clues(state_t *st,
                const int32_t *clues) {
  line_t rlines[SIZE];
  line_t clines[SIZE];
  int32_t i;
  memcpy(st->m_clues, clues, sizeof(int32_t)*SIZE*4);

  for (i = 0; i < SIZE; ++i) {
    clines[i].ix = rlines[i].ix = i;
    clines[i].clN = rlines[i].clN = 0;

    clines[i].clN += clues[i] == 0 ? 0 : 1;
    clines[i].clN += clues[SIZE*3 - 1 - i] == 0 ? 0 : 1;
    rlines[i].clN += clues[SIZE+i] == 0 ? 0 : 1;
    rlines[i].clN += clues[SIZE*4 - 1 - i] == 0 ? 0 : 1;
  }

  qsort(rlines, SIZE, sizeof(line_t), line_cmp_desc);
  qsort(clines, SIZE, sizeof(line_t), line_cmp_desc);

  for (i = 0; i < SIZE; ++i) {
    st->m_col_indexes[i] = clines[i].ix;
    st->m_row_indexes[i] = rlines[i].ix;
  }
}
///////////////////////////////////////////////////////

static const int32_t dir_steps[4] = {-1, 1, -1, 1};
static const int32_t dir_starts[4] = {SIZE-1, 0, SIZE-1, 0};

int32_t row_n(const state_t *st,
              int32_t r,
              direction_t d) {
  assert(st);
  assert(r >= 0 && r < SIZE);
  assert(d == left || d == right);
  int32_t cs = dir_starts[d];
  int32_t ce = cs + (SIZE * dir_steps[d]);
  int32_t max = st->m_mtx[r][cs];
  int32_t n = max != 0 ? 1 : 0; //we can see first one
  cs += dir_steps[d];
  for(; cs != ce; cs += dir_steps[d]) {
    if (st->m_mtx[r][cs] <= max)
      continue;
    ++n;
    max = st->m_mtx[r][cs];
  }
  return n;
}
///////////////////////////////////////////////////////

int32_t row_n2(const state_t *st,
               int32_t r,
               direction_t d) {
  assert(st);
  assert(r >= 0 && r < SIZE);
  assert(d == left || d == right);
  int32_t cs = dir_starts[d];
  int32_t ce = cs + (SIZE * dir_steps[d]);
  int32_t max = st->m_mtx[r][cs];
  int32_t n = max != 0 ? 1 : 0; //we can see first one

  if (max == 0)
    return 0; //we can't predict on this step.

  cs += dir_steps[d];
  for(; cs != ce; cs += dir_steps[d]) {
    if (st->m_mtx[r][cs] == 0) break;
    if (st->m_mtx[r][cs] <= max)
      continue;
    ++n;
    max = st->m_mtx[r][cs];
  }
  return n;
}
///////////////////////////////////////////////////////

int32_t col_n(const state_t *st,
              int32_t c,
              direction_t d) {
  assert(st);
  assert(c >= 0 && c < SIZE);
  assert(d == up|| d == down);
  int32_t rs = dir_starts[d];
  int32_t re = rs + (SIZE * dir_steps[d]);
  int32_t max = st->m_mtx[rs][c];
  int32_t n = max != 0 ? 1 : 0; //we can see first one
  rs += dir_steps[d];
  for(; rs != re; rs += dir_steps[d]) {
    if (st->m_mtx[rs][c] <= max)
      continue;
    ++n;
    max = st->m_mtx[rs][c];
  }
  return n;
}
///////////////////////////////////////////////////////

int32_t col_n2(const state_t *st,
               int32_t c,
               direction_t d) {
  assert(st);
  assert(c >= 0 && c < SIZE);
  assert(d == up|| d == down);
  int32_t rs = dir_starts[d];
  int32_t re = rs + (SIZE * dir_steps[d]);
  int32_t max = st->m_mtx[rs][c];
  int32_t n = max != 0 ? 1 : 0; //we can see first one
  if (max == 0)
    return 0;

  rs += dir_steps[d];
  for(; rs != re; rs += dir_steps[d]) {
    if (st->m_mtx[rs][c] == 0) break;
    if (st->m_mtx[rs][c] <= max)
      continue;
    ++n;
    max = st->m_mtx[rs][c];
  }
  return n;
}
///////////////////////////////////////////////////////

bool set_item(state_t *st,
              int32_t r,
              int32_t c,
              int32_t it) {
  assert(c < SIZE && c >= 0);
  assert(r < SIZE && r >= 0);
  assert(it >= 1 && it <= SIZE);
  assert(st);
  --it;
  if (st->m_col_mask[c] & (1 << it))
    return false;
  if (st->m_row_mask[r] & (1 << it))
    return false;

  st->m_col_mask[c] |= (1 << it);
  st->m_row_mask[r] |= (1 << it);
  st->m_mtx[r][c] = ++it;
  return true;
}
///////////////////////////////////////////////////////

void clear_item(state_t *st,
                int32_t r,
                int32_t c) {
  assert(c < SIZE && c >= 0);
  assert(r < SIZE && r >= 0);
  assert(st);
  st->m_col_mask[c] &= ~(1 << (st->m_mtx[r][c]-1));
  st->m_row_mask[r] &= ~(1 << (st->m_mtx[r][c]-1));
  st->m_mtx[r][c] = 0;
}
///////////////////////////////////////////////////////

bool row_is_ok(const state_t *st,
               int32_t r) {
  int32_t q;
  //row l->r
  if ((q = st->m_clues[SIZE * 4 - r - 1])) {
    if (row_n(st, r, right) != q)
      return false;
  }
  //row r->l
  if ((q = st->m_clues[SIZE + r])) {
    if (row_n(st, r, left) != q)
      return false;
  }
  return true;
}
///////////////////////////////////////////////////////

bool col_is_ok(const state_t *st,
               int32_t c) {
  int32_t q;
  //col u->d
  if ((q = st->m_clues[c])) {
    if (col_n(st, c, down) != q)
      return false;
  }
  //col d->u
  if ((q = st->m_clues[SIZE * 3 - c - 1])) {
    if (col_n(st, c, up) != q)
      return false;
  }
  return true;
}
///////////////////////////////////////////////////////

bool state_is_final(const state_t *st) {
  int32_t qi;
  for (qi=0; qi < SIZE; ++qi) {
    if (!row_is_ok(st, qi))
      return false;
    if (!col_is_ok(st, qi))
      return false;
  }
  return true;
}
///////////////////////////////////////////////////////

void print_state(const state_t *st) {
  int32_t r, c, qi;
  printf("\n");

  for (qi = 0; qi < SIZE; ++qi)
    printf("\t%d", st->m_clues[qi]);
  printf("\n");

  for (r = 0; r < SIZE; ++r) {
    printf("%d", st->m_clues[SIZE*4 - 1 - r]);
    for (c = 0; c < SIZE; ++c) {
      printf("\t%d", st->m_mtx[r][c]);
    }
    printf("\t%d\n", st->m_clues[SIZE+r]);
  }

  for (qi = 0; qi < SIZE; ++qi)
    printf("\t%d", st->m_clues[SIZE*3 - 1 - qi]);

  printf("\n");
}
///////////////////////////////////////////////////////

bool check_clues(const state_t *st, int32_t r, int32_t c) {
  int32_t clue, cnt;

  if (st->m_row_mask[r] == FULL) { //check full row
    if ((clue = st->m_clues[SIZE*4 - 1 - r]) && (cnt = row_n(st, r, right)) != clue)
      return false;
    if ((clue = st->m_clues[SIZE + r]) && (cnt = row_n(st, r, left)) != clue)
      return false;
  } else { //check not full row
    if ((clue = st->m_clues[SIZE*4 - 1 - r]) && (cnt = row_n2(st, r, right) > clue))
      return false;
    if ((clue = st->m_clues[SIZE + r]) && (cnt = row_n2(st, r, left)) > clue)
      return false;
  }

  if (st->m_col_mask[c] == FULL) { //check full col
    if ((clue = st->m_clues[c]) && (cnt = col_n(st, c, down)) != clue)
      return false;
    if ((clue = st->m_clues[SIZE*3 - 1 - c]) && (cnt = col_n(st, c, up)) != clue)
      return false;
  } else { //check not full col
    if ((clue = st->m_clues[c]) && (cnt = col_n2(st, c, down)) > clue)
      return false;
    if ((clue = st->m_clues[SIZE*3 - 1 - c]) && (cnt = col_n2(st, c, up)) > clue)
      return false;
  }
  return true;
}
///////////////////////////////////////////////////////

void solve_edge_row(state_t *st,
                    int32_t r) {
  assert(st);
  assert(r >= 0 && r < SIZE);
  int32_t cL, cR, i;
  cL = st->m_clues[SIZE*4 - 1 - r];
  cR = st->m_clues[SIZE + r];

  if (cL + cR == SIZE + 1) {
    set_item(st, r, cL-1, SIZE);
  }

  if (cL == SIZE) {
    for (i = 0; i < SIZE; ++i)
      set_item(st, r, i, i+1);
  }

  if (cR == SIZE) {
    for (i = 0; i < SIZE; ++i)
      set_item(st, r, i, SIZE-i);
  }

  if (cL == 1)
    set_item(st, r, 0, SIZE);
  if (cR == 1)
    set_item(st, r, SIZE-1, SIZE);
}
///////////////////////////////////////////////////////

void solve_edge_col(state_t *st,
                    int32_t c) {
  assert(st);
  assert(c >= 0 && c < SIZE);
  int32_t cU, cD, i;
  cU = st->m_clues[c];
  cD = st->m_clues[SIZE*3 - 1 - c];

  if (cU + cD == SIZE + 1) {
    set_item(st, cU-1, c, SIZE);
  }

  if (cU == SIZE) {
    for (i = 0; i < SIZE; ++i)
      set_item(st, i, c, i+1);
  }

  if (cD == SIZE) {
    for (i = 0; i < SIZE; ++i)
      set_item(st, i, c, SIZE-i);
  }

  if (cU == 1)
    set_item(st, 0, c, SIZE);
  if (cD == 1)
    set_item(st, SIZE-1, c, SIZE);
}
///////////////////////////////////////////////////////

void solve_edge(state_t* st) {
  for (int32_t qi = 0; qi < SIZE; ++qi) {
    solve_edge_row(st, qi);
    solve_edge_col(st, qi);
  }
}
///////////////////////////////////////////////////////

//this will skip not 0 items.
bool solve_cleared(state_t *st,
                   int32_t ri,
                   int32_t ci,
                   int32_t it) {
  int32_t r, c;
  if (ci == SIZE) {
    ++ri; ci = 0;
  }
  r = st->m_row_indexes[ri];
  c = st->m_col_indexes[ci];

  if (ri == SIZE)
    return state_is_final(st);
  if (it == 0)
    return false;
  if (st->m_mtx[r][c])
    return solve_cleared(st, ri, ci+1, SIZE);

  do {
    if (!set_item(st, r, c, it))
      break;
    if (!check_clues(st, r, c))
      break;
    if (solve_cleared(st, ri, ci+1, SIZE))
      return true;
  } while(0);

  clear_item(st, r, c);
  return solve_cleared(st, ri, ci, it-1);
}
///////////////////////////////////////////////////////

//It's not smart, there is wrong backtracking mechanism.
//It brutforces all possible values from max to min value
//instead of trying to set max, then max-1, then max-2 etc.
//to whole field.
static bool solve_smart(state_t *st) {
  solve_edge(st);  
  return solve_cleared(st, 0, 0, SIZE);
}
///////////////////////////////////////////////////////

int32_t **SolvePuzzle(const int32_t *clues) {
  int32_t **result = malloc(SIZE * sizeof (int32_t*));
  bool solved = false;
  state_t st = new_state();
  int32_t r;
  for (r = 0; r < SIZE; ++r)
    result[r] = malloc(SIZE * sizeof(int32_t));

  init_clues(&st, clues);

  solved = solve_smart(&st);
  if (solved) {
    print_state(&st);
    for (r = 0; r < SIZE; ++r)
      memcpy(result[r], st.m_mtx[r], SIZE*sizeof (int32_t));
    return result;
  }
  return NULL;
}
