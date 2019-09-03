#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "skyscrapers.h"

#define SIZE 7
#define FULL 0x7f

typedef struct state {
  //main
  int32_t m_mtx[SIZE][SIZE];
  int32_t m_clues[SIZE*4];

  //aux
  uint32_t m_possibilities[SIZE][SIZE];
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
static void init_possibilities(state_t *st); //should be called AFTER init_clues method
static uint32_t get_possibility(int clue, int distance);

static int row_full_n(const state_t *st, int32_t r, direction_t d);
static int col_full_n(const state_t *st, int32_t c, direction_t d);
static int row_partial_n(const state_t *st, int32_t r, direction_t d);
static int col_partial_n(const state_t *st, int32_t c, direction_t d);

static bool set_item(state_t *st, int32_t r, int32_t c, int32_t it);
static void clear_item(state_t *st, int32_t r, int32_t c);

static bool state_is_final(const state_t *st);
static bool row_is_ok(const state_t *st, int32_t r);
static bool col_is_ok(const state_t *st, int32_t c);

static bool check_clues(const state_t *st, int32_t r, int32_t c);

static int32_t next_val(state_t *st, int32_t ri, int32_t ci, int32_t cit);
static bool solve(state_t *st, int32_t ri, int32_t ci, int32_t it);


static void print_state(const state_t *st);
static void print_possibilities(const state_t *st);

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

uint32_t get_possibility(int clue, int distance) {
  //set 1 to all bits from 0 to SIZE
  uint32_t possibility = (uint32_t) (~(0xffffffff << SIZE));

  if (clue > distance + 1) {
    possibility >>= (clue - distance - 1);
  }

  switch (clue) {
    case 1:
      if (distance) {
        possibility &= (uint32_t) ~(0x01 << (SIZE - 1)); //set MSB to 0
      } else {
        possibility = 0x01 << (SIZE - 1); //set MSB to 1
      }
      break;

    case SIZE:
      possibility = 0x01 << distance;
      break;

    default:
      break;
  }

  return possibility;
}
///////////////////////////////////////////////////////

void init_possibilities(state_t *st) {
  for (int y = 0; y < SIZE; ++y) {
    for (int x = 0; x < SIZE; ++x) {
      st->m_possibilities[y][x] =  get_possibility(st->m_clues[4 * SIZE - 1 - y], x); //left
      st->m_possibilities[y][x] &= get_possibility(st->m_clues[SIZE + y], SIZE - 1 - x); //right
      st->m_possibilities[y][x] &= get_possibility(st->m_clues[x], y); //top
      st->m_possibilities[y][x] &= get_possibility(st->m_clues[3 * SIZE - 1 - x], SIZE - 1 - y); //bottom
    }
  }
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

int32_t row_full_n(const state_t *st,
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

int32_t row_partial_n(const state_t *st,
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

int32_t col_full_n(const state_t *st,
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

int32_t col_partial_n(const state_t *st,
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
    if (row_full_n(st, r, right) != q)
      return false;
  }
  //row r->l
  if ((q = st->m_clues[SIZE + r])) {
    if (row_full_n(st, r, left) != q)
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
    if (col_full_n(st, c, down) != q)
      return false;
  }
  //col d->u
  if ((q = st->m_clues[SIZE * 3 - c - 1])) {
    if (col_full_n(st, c, up) != q)
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

void print_possibilities(const state_t *st) {
  int32_t r, c, qi;
  printf("\n");

  for (qi = 0; qi < SIZE; ++qi)
    printf("\t%d", st->m_clues[qi]);
  printf("\n");

  for (r = 0; r < SIZE; ++r) {
    printf("%d", st->m_clues[SIZE*4 - 1 - r]);
    for (c = 0; c < SIZE; ++c) {
      printf("\t%02x", st->m_possibilities[r][c]);
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
    if ((clue = st->m_clues[SIZE*4 - 1 - r]) && (cnt = row_full_n(st, r, right)) != clue)
      return false;
    if ((clue = st->m_clues[SIZE + r]) && (cnt = row_full_n(st, r, left)) != clue)
      return false;
  } else { //check not full row
    if ((clue = st->m_clues[SIZE*4 - 1 - r]) && (cnt = row_partial_n(st, r, right) > clue))
      return false;
    if ((clue = st->m_clues[SIZE + r]) && (cnt = row_partial_n(st, r, left)) > clue)
      return false;
  }

  if (st->m_col_mask[c] == FULL) { //check full col
    if ((clue = st->m_clues[c]) && (cnt = col_full_n(st, c, down)) != clue)
      return false;
    if ((clue = st->m_clues[SIZE*3 - 1 - c]) && (cnt = col_full_n(st, c, up)) != clue)
      return false;
  } else { //check not full col
    if ((clue = st->m_clues[c]) && (cnt = col_partial_n(st, c, down)) > clue)
      return false;
    if ((clue = st->m_clues[SIZE*3 - 1 - c]) && (cnt = col_partial_n(st, c, up)) > clue)
      return false;
  }
  return true;
}
///////////////////////////////////////////////////////

static uint32_t MSB_position(uint32_t v) {
  static const int8_t LogTable256[256] =
  {
  #define LT(n) n, n, n, n, n, n, n, n, n, n, n, n, n, n, n, n
    -1,
    0,
    1, 1,
    2, 2, 2, 2,
    3, 3, 3, 3, 3, 3, 3, 3,
    LT(4),
    LT(5), LT(5),
    LT(6), LT(6), LT(6), LT(6),
    LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7)
  };
  uint32_t r;     // r will be lg(v)
  uint32_t t, tt; // temporaries

  if ((tt = v >> 16)) {
    r = (t = tt >> 8) ? 24 + LogTable256[t] : 16 + LogTable256[tt];
  } else {
    r = (t = v >> 8) ? 8 + LogTable256[t] : LogTable256[v];
  }
  return r;
}
///////////////////////////////////////////////////////

int32_t next_val(state_t *st, int32_t ri, int32_t ci, int32_t cit) {
  int r, c;

  if (ci == SIZE) {
    ++ri; ci = 0;
  }

  r = st->m_row_indexes[ri];
  c = st->m_col_indexes[ci];

  uint32_t pos = st->m_possibilities[r][c];
  if (cit) {
    pos &= ~(0xffffffff << (cit-1));
  }
  return MSB_position(pos) + 1;
}
///////////////////////////////////////////////////////

bool solve(state_t *st,
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

  do {
    if (!set_item(st, r, c, it))
      break;
    if (!check_clues(st, r, c))
      break;
    if (solve(st, ri, ci+1, next_val(st, ri, ci+1, 0)))
      return true;
  } while(0);

  clear_item(st, r, c);
  int nit = next_val(st, ri, ci, it);
  return solve(st, ri, ci,  nit);
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
  init_possibilities(&st);
  solved = solve(&st, 0, 0, next_val(&st, 0, 0, 0));
  if (solved) {
    for (r = 0; r < SIZE; ++r)
      memcpy(result[r], st.m_mtx[r], SIZE*sizeof (int32_t));
    return result;
  }
  return NULL;
}
