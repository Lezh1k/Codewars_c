#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "skyscrapers.h"

#define SIZE 7
#define FULL (~(0xffffffffu << SIZE))

typedef enum side {
  top=0,
  right,
  bottom,
  left
} side_t;

static const int32_t dir_steps[4] = {-1, 1, 1, -1};
static const int32_t dir_starts[4] = {SIZE-1, 0, 0, SIZE-1};

typedef struct state {
  //main
  int32_t m_mtx[SIZE][SIZE];
  int32_t m_clues[4][SIZE];
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
  int32_t clues_count;
} line_t;

static int line_cmp_desc(const void* l1, const void* l2) {
  const line_t *pl2, *pl1;
  pl2 = (const line_t*)l2;
  pl1 = (const line_t*)l1;

  if (pl2->clues_count == pl1->clues_count)
    return pl1->ix - pl2->ix;
  return pl2->clues_count - pl1->clues_count;
}
///////////////////////////////////////////////////////
///////////////////////////////////////////////////////
///////////////////////////////////////////////////////

static state_t new_state(const int32_t *clues);

static void init_clues(state_t *st, const int *clues);
static void init_possibilities(state_t *st); //should be called AFTER init_clues method

static uint32_t get_possibility(int clue, int distance);

static int row_full_n(const state_t *st, int32_t r, side_t d);
static int col_full_n(const state_t *st, int32_t c, side_t d);
static int row_partial_n(const state_t *st, int32_t r, side_t d);
static int col_partial_n(const state_t *st, int32_t c, side_t d);

static bool set_item(state_t *st, int32_t r, int32_t c, int32_t it);
static void clear_item(state_t *st, int32_t r, int32_t c);

static bool row_is_ok(const state_t *st, int32_t r);
static bool col_is_ok(const state_t *st, int32_t c);

static bool check_clues(const state_t *st, int32_t r, int32_t c);

static int32_t next_val(state_t *st, int32_t ri, int32_t ci, int32_t cit);
static bool solve(state_t *st, int32_t ri, int32_t ci, int32_t it);


static void print_state(const state_t *st);
static void print_possibilities(const state_t *st);

state_t new_state(const int32_t *clues) {
  int32_t r;
  state_t st;
  for (r = 0; r < SIZE; ++r) {
    memset(st.m_mtx[r], 0, sizeof(int32_t)*SIZE);
  }
  memset(st.m_col_mask, 0, sizeof (int32_t)*SIZE);
  memset(st.m_row_mask, 0, sizeof (int32_t)*SIZE);

  init_clues(&st, clues);
  init_possibilities(&st);
  return st;
}
///////////////////////////////////////////////////////

uint32_t get_possibility(int clue, int distance) {
  //set 1 to all bits from 0 to SIZE.
  uint32_t possibility = ~(0xffffffffu << SIZE);
  // building can't be higher than clue in interval [0, distance].
  // SO!
  if (clue > distance + 1) {
    possibility >>= (clue - distance - 1);
  }

  if (clue == 1) {
    if (distance) {
      possibility &= ~(0x01 << (SIZE - 1)); //else set MSB to 0
    } else {
      possibility = 0x01 << (SIZE - 1); //set MSB to 1 if distance == 0
    }
  }

  if (clue == SIZE) {
    possibility = 0x01 << distance;
  }
  // maybe we have more options here? I'm not sure :(
  return possibility;
}
///////////////////////////////////////////////////////

void init_possibilities(state_t *st) {
  for (int y = 0; y < SIZE; ++y) {
    for (int x = 0; x < SIZE; ++x) {
      st->m_possibilities[y][x] = get_possibility(st->m_clues[left][y], x);
      st->m_possibilities[y][x] &= get_possibility(st->m_clues[right][y], SIZE - 1 - x);

      st->m_possibilities[y][x] &= get_possibility(st->m_clues[top][x], y);
      st->m_possibilities[y][x] &= get_possibility(st->m_clues[bottom][x], SIZE - 1 - y);
    }
  }
}
///////////////////////////////////////////////////////

void init_clues(state_t *st,
                const int32_t *clues) {
  line_t rlines[SIZE];
  line_t clines[SIZE];

  for (int i = 0; i < SIZE; ++i) {
    st->m_clues[top][i] = clues[i];
    st->m_clues[right][i] = clues[SIZE+i];
    st->m_clues[bottom][i] = clues[3*SIZE - 1 - i];
    st->m_clues[left][i] = clues[4*SIZE - 1 - i];
  }

  for (int i = 0; i < SIZE; ++i) {
    clines[i].ix = rlines[i].ix = i;
    clines[i].clues_count = rlines[i].clues_count = 0;

    clines[i].clues_count += st->m_clues[top][i] == 0 ? 0 : 1;
    clines[i].clues_count += st->m_clues[bottom][i] == 0 ? 0 : 1;
    rlines[i].clues_count += st->m_clues[right][i] == 0 ? 0 : 1;
    rlines[i].clues_count += st->m_clues[left][i] == 0 ? 0 : 1;
  }

  // we need to solve lines with biggest amount of not null clues first.
  qsort(rlines, SIZE, sizeof(line_t), line_cmp_desc);
  qsort(clines, SIZE, sizeof(line_t), line_cmp_desc);

  for (int i = 0; i < SIZE; ++i) {
    st->m_col_indexes[i] = clines[i].ix;
    st->m_row_indexes[i] = rlines[i].ix;
  }
}
///////////////////////////////////////////////////////

int32_t row_full_n(const state_t *st,
                   int32_t r,
                   side_t d) {
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
                      side_t d) {
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
    if (st->m_mtx[r][cs] == 0)
      break;
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
                   side_t d) {
  assert(st);
  assert(c >= 0 && c < SIZE);
  assert(d == top|| d == bottom);
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
                      side_t d) {
  assert(st);
  assert(c >= 0 && c < SIZE);
  assert(d == top|| d == bottom);
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
  assert(st);
  assert(c < SIZE && c >= 0);
  assert(r < SIZE && r >= 0);
  assert(it >= 1 && it <= SIZE);
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
  assert(st);
  assert(c < SIZE && c >= 0);
  assert(r < SIZE && r >= 0);
  st->m_col_mask[c] &= ~(1 << (st->m_mtx[r][c]-1));
  st->m_row_mask[r] &= ~(1 << (st->m_mtx[r][c]-1));
  st->m_mtx[r][c] = 0;
}
///////////////////////////////////////////////////////

bool row_is_ok(const state_t *st,
               int32_t r) {
  int32_t q;
  //row l->r
  if ((q = st->m_clues[left][r])) {
    if (row_full_n(st, r, right) != q)
      return false;
  }
  //row r->l
  if ((q = st->m_clues[right][r])) {
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
  if ((q = st->m_clues[top][c])) {
    if (col_full_n(st, c, bottom) != q)
      return false;
  }
  //col d->u
  if ((q = st->m_clues[bottom][c])) {
    if (col_full_n(st, c, top) != q)
      return false;
  }
  return true;
}
///////////////////////////////////////////////////////

void print_state(const state_t *st) {
  int32_t r, c, qi;
  printf("\n");

  for (qi = 0; qi < SIZE; ++qi)
    printf("\t%d", st->m_clues[top][qi]);
  printf("\n");

  for (r = 0; r < SIZE; ++r) {
    printf("%d", st->m_clues[left][r]);
    for (c = 0; c < SIZE; ++c) {
      printf("\t%d", st->m_mtx[r][c]);
    }
    printf("\t%d\n", st->m_clues[right][r]);
  }

  for (qi = 0; qi < SIZE; ++qi)
    printf("\t%d", st->m_clues[bottom][qi]);
  printf("\n");
}
///////////////////////////////////////////////////////

void print_possibilities(const state_t *st) {
  int32_t r, c, qi;
  printf("\n");
  for (qi = 0; qi < SIZE; ++qi)
    printf("\t%d", st->m_clues[top][qi]);
  printf("\n");
  for (r = 0; r < SIZE; ++r) {
    printf("%d", st->m_clues[left][r]);
    for (c = 0; c < SIZE; ++c) {
      printf("\t%02x", st->m_possibilities[r][c]);
    }
    printf("\t%d\n", st->m_clues[right][r]);
  }
  for (qi = 0; qi < SIZE; ++qi)
    printf("\t%d", st->m_clues[bottom][qi]);
  printf("\n");
}
///////////////////////////////////////////////////////

bool check_clues(const state_t *st, int32_t r, int32_t c) {
  if (st->m_row_mask[r] == FULL) { //check full row
    if (st->m_clues[left][r] && row_full_n(st, r, right) != st->m_clues[left][r])
      return false;
    if (st->m_clues[right][r] && row_full_n(st, r, left) != st->m_clues[right][r])
      return false;
  } else { //check not full row
    if (st->m_clues[left][r] && row_partial_n(st, r, right) > st->m_clues[left][r])
      return false;
    if (st->m_clues[right][r] && row_partial_n(st, r, left) > st->m_clues[right][r])
      return false;
  }

  if (st->m_col_mask[c] == FULL) { //check full col
    if (st->m_clues[top][c] && col_full_n(st, c, bottom) != st->m_clues[top][c])
      return false;
    if (st->m_clues[bottom][c] && col_full_n(st, c, top) != st->m_clues[bottom][c])
      return false;
  } else { //check not full col
    if (st->m_clues[top][c] && col_partial_n(st, c, bottom) > st->m_clues[top][c])
      return false;
    if (st->m_clues[bottom][c] && col_partial_n(st, c, top) > st->m_clues[bottom][c])
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

///
/// \brief next_val
/// \param st - state
/// \param ri - row index
/// \param ci - column index
/// \param cit - cell item
/// \return
///
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
    return true;

  if (it == 0)
    return false;

  do {
    if (!set_item(st, r, c, it))
      break;

    /* here we need somehow rebuild our possibilities.
     * have no idea how */

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
  int32_t **result;
  state_t st = new_state(clues);
//  print_possibilities(&st);

  if (!solve(&st, 0, 0, next_val(&st, 0, 0, 0)))
    return NULL;

//  print_state(&st);
  result = malloc(SIZE * sizeof (int32_t*));
  for (int32_t r = 0; r < SIZE; ++r) {
    result[r] = malloc(SIZE * sizeof(int32_t));
    memcpy(result[r], st.m_mtx[r], SIZE*sizeof (int32_t));
  }
  return result;
}
