#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "skyscapers.h"

#define SIZE 6
#define FULL 0x7e

typedef struct state {
  //main
  int m_mtx[SIZE][SIZE];
  int m_clues[SIZE*4];

  //aux
  int m_col_mask[SIZE];
  int m_row_mask[SIZE];

  //in this order we should solve
  int m_row_indexes[SIZE];
  int m_col_indexes[SIZE];
} state_t;
///////////////////////////////////////////////////////

typedef struct line {
  int ix;
  int clN;
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

static int row_n(const state_t *st, int r, direction_t d);
static int col_n(const state_t *st, int c, direction_t d);

static bool set_item(state_t *st, int r, int c, int it);
static void clear_item(state_t *st, int r, int c);

static bool state_is_final(const state_t *st);
static bool check_clues(const state_t *st, int r, int c);

static bool row_is_ok(const state_t *st, int r);
static bool col_is_ok(const state_t *st, int c);

static void print_state(const state_t *st);

state_t new_state() {
  int r;
  state_t st;
  for (r = 0; r < SIZE; ++r)
    memset(st.m_mtx[r], 0, sizeof(int)*SIZE);
  memset(st.m_col_mask, 0, sizeof (int)*SIZE);
  memset(st.m_row_mask, 0, sizeof (int)*SIZE);
  return st;
}
///////////////////////////////////////////////////////

void init_clues(state_t *st,
               const int *clues) {
  line_t rlines[SIZE];
  line_t clines[SIZE];
  int i;
  memcpy(st->m_clues, clues, sizeof(int)*SIZE*4);

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

static const int dir_steps[4] = {-1, 1, -1, 1};
static const int dir_starts[4] = {SIZE-1, 0, SIZE-1, 0};

int row_n(const state_t *st,
          int r,
          direction_t d) {
  assert(st);
  assert(r >= 0 && r < SIZE);
  assert(d == left || d == right);
  int cs = dir_starts[d];
  int ce = cs + (SIZE * dir_steps[d]);
  int max = st->m_mtx[r][cs];
  int n = max != 0 ? 1 : 0; //we can see first one
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

int col_n(const state_t *st,
          int c,
          direction_t d) {
  assert(st);
  assert(c >= 0 && c < SIZE);
  assert(d == up|| d == down);
  int rs = dir_starts[d];
  int re = rs + (SIZE * dir_steps[d]);
  int max = st->m_mtx[rs][c];
  int n = max != 0 ? 1 : 0; //we can see first one
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

bool set_item(state_t *st,
              int r,
              int c,
              int it) {
  assert(c < SIZE && c >= 0);
  assert(r < SIZE && r >= 0);
  assert(it >= 1 && it <= SIZE);
  assert(st);

  if (st->m_col_mask[c] & (1 << it))
    return false;
  if (st->m_row_mask[r] & (1 << it))
    return false;

  st->m_col_mask[c] |= (1 << it);
  st->m_row_mask[r] |= (1 << it);
  st->m_mtx[r][c] = it;
  return true;
}
///////////////////////////////////////////////////////

void clear_item(state_t *st,
                int r,
                int c) {
  assert(c < SIZE && c >= 0);
  assert(r < SIZE && r >= 0);
  assert(st);
  st->m_col_mask[c] &= ~(1 << st->m_mtx[r][c]);
  st->m_row_mask[r] &= ~(1 << st->m_mtx[r][c]);
  st->m_mtx[r][c] = 0;
}
///////////////////////////////////////////////////////

bool row_is_ok(const state_t *st,
               int r) {
  int q;
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
               int c) {
  int q;
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
  int qi;
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
  int r, c, qi;
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

bool check_clues(const state_t *st, int r, int c) {
  int clue, cnt;
  if (st->m_row_mask[r] == FULL) { //check full row
    if ((clue = st->m_clues[SIZE*4 - 1 - r]) && (cnt = row_n(st, r, right)) != clue)
      return false;
    if ((clue = st->m_clues[SIZE + r]) && (cnt = row_n(st, r, left)) != clue)
      return false;
  }

  if (st->m_col_mask[c] == FULL) { //check full col
    if ((clue = st->m_clues[c]) && (cnt = col_n(st, c, down)) != clue)
      return false;
    if ((clue = st->m_clues[SIZE*3 - 1 - c]) && (cnt = col_n(st, c, up)) != clue)
      return false;
  }
  return true;
}
///////////////////////////////////////////////////////

static void solve_edge_row(state_t *st,
                           int r,
                           int d,
                           int clue) {
  assert(st);
  assert(r >= 0 && r < SIZE);
  assert(d == left || d == right);
  assert(clue == SIZE || clue == 1);
  int cs = dir_starts[d];
  int ce = cs + (SIZE * dir_steps[d]);
  int v, vs;

  if (clue == 1) {
    set_item(st, r, cs, SIZE);
    return;
  }
  v = vs = 1;
  for(; cs != ce; cs += dir_steps[d], v += vs)
    set_item(st, r, cs, v);
}
///////////////////////////////////////////////////////

static void solve_edge_col(state_t *st,
                           int c,
                           int d,
                           int clue) {
  assert(st);
  assert(c >= 0 && c < SIZE);
  assert(d == up|| d == down);
  assert(clue == 1 || clue == SIZE);
  int rs = dir_starts[d];
  int re = rs + (SIZE * dir_steps[d]);
  int v, vs;

  if (clue == 1) {
    set_item(st, rs, c, SIZE);
    return;
  }
  v = vs = 1;
  for(; rs != re; rs += dir_steps[d], v += vs)
    set_item(st, rs, c, v);
}
///////////////////////////////////////////////////////

static void solve_edge(state_t* st) {
  int qi;
  //top clues
  for (qi = 0; qi < SIZE; ++qi) {
    if (st->m_clues[qi] != 1 && st->m_clues[qi] != SIZE)
      continue;
    solve_edge_col(st, qi, down, st->m_clues[qi]);
  }
  //right clues
  for (qi = SIZE; qi < SIZE*2; ++qi) {
    if (st->m_clues[qi] != 1 && st->m_clues[qi] != SIZE)
      continue;
    solve_edge_row(st, qi-SIZE, left, st->m_clues[qi]);
  }
  //bottom clues
  for (qi = SIZE*2; qi < SIZE*3; ++qi) {
    if (st->m_clues[qi] != 1 && st->m_clues[qi] != SIZE)
      continue;
    solve_edge_col(st, SIZE*3 - 1 - qi, up, st->m_clues[qi]);
  }
  //left clues
  for (qi = SIZE*3; qi < SIZE*4; ++qi) {
    if (st->m_clues[qi] != 1 && st->m_clues[qi] != SIZE)
      continue;
    solve_edge_row(st, SIZE*4 - 1 - qi, right, st->m_clues[qi]);
  }
}
///////////////////////////////////////////////////////

//this will skip not 0 items.
static bool solve_cleared(state_t *st,
                          int ri,
                          int ci,
                          int it) {
  int r, c;
  if (ci == SIZE) {
    ++ri; ci = 0;
  }
  r = st->m_row_indexes[ri];
  c = st->m_col_indexes[ci];

  if (ri == SIZE)
    return state_is_final(st);  
  if (it == SIZE+1)
    return false;
  if (st->m_mtx[r][c])
    return solve_cleared(st, ri, ci+1, 1);

  do {
    if (!set_item(st, r, c, it))
      break;    
    if (!check_clues(st, r, c))
      break;
    if (solve_cleared(st, ri, ci+1, 1))
      return true;
  } while(0);

  clear_item(st, r, c);
  return solve_cleared(st, ri, ci, it+1);
}
///////////////////////////////////////////////////////

static bool solve_smart(state_t *st) {
  solve_edge(st);
  return solve_cleared(st, 0, 0, 1);
}
///////////////////////////////////////////////////////

int **SolvePuzzle(const int *clues) {
  int **result = malloc(SIZE * sizeof (int*));
  bool solved = false;
  state_t st = new_state();
  int r;
  for (r = 0; r < SIZE; ++r)
    result[r] = malloc(SIZE * sizeof(int));

  init_clues(&st, clues);  
  solved = solve_smart(&st);
  if (solved) {
    print_state(&st);
    for (r = 0; r < SIZE; ++r)
      memcpy(result[r], st.m_mtx[r], SIZE*sizeof (int));
    return result;
  }
  return NULL;
}
