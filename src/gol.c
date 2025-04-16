#include "gol.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
  cell_curr = 0,
  cell_next = 1,
};

enum {
  cell_dead = 0,
  cell_alive,
};

typedef union cell {
  int8_t b[4];
  int32_t v;
} cell_t;

typedef struct field {
  cell_t *data;
  int32_t cols;
  int32_t rows;
} field_t;
//////////////////////////////////////////////////////////////

int **get_generation(const int32_t **cells, int32_t generations, int32_t *ptr_r,
                     int32_t *ptr_c);

static field_t game_create(const int32_t **cells, int32_t rows, int32_t cols);
static void game_free(field_t *f);

static void game_iteration(field_t *pField);
static void game_calc_cell_status(field_t *f, int32_t row, int32_t col);
static int32_t game_not_null_neighbors(const field_t *field, int32_t row,
                                       int32_t col);
static void game_expand_field(field_t *f);
static void game_shrink_field(field_t *f);

#if 1
static void game_print_field(const field_t *f) {
  for (int r = 0; r < f->rows; ++r) {
    for (int c = 0; c < f->cols; ++c) {
      printf("%x,", f->data[r * f->cols + c].v);
    }
    printf("\n");
  }
  printf("*****************************\n");
}
//////////////////////////////////////////////////////////////
#endif

int **get_generation(const int32_t **cells, int32_t generations, int32_t *ptr_r,
                     int32_t *ptr_c) {
  field_t f = game_create(cells, *ptr_r, *ptr_c);
  game_print_field(&f);
  game_expand_field(&f);
  for (int32_t gen = 0; gen < generations; ++gen) {
    game_iteration(&f);
  }
  game_shrink_field(&f);
  game_print_field(&f);

  int **res = malloc(sizeof(int *) * f.rows);
  for (int r = 0; r < f.rows; ++r) {
    res[r] = malloc(sizeof(int) * f.cols);
    for (int c = 0; c < f.cols; ++c) {
      res[r][c] = f.data[r * f.cols + c].b[cell_curr];
    }
  }

  *ptr_r = f.rows;
  *ptr_c = f.cols;
  game_free(&f);
  return res;
}
//////////////////////////////////////////////////////////////

field_t game_create(const int32_t **cells, int32_t rows, int32_t cols) {
  field_t res = {.cols = cols, .rows = rows, .data = NULL};
  res.data = malloc(sizeof(cell_t) * rows * cols);
  for (int32_t r = 0; r < rows; ++r) {
    cell_t *ptr_r = &res.data[r * res.cols];
    memcpy(ptr_r, cells[r], cols * sizeof(cell_t));
  }
  return res;
}
//////////////////////////////////////////////////////////////

void game_free(field_t *f) {
  free(f->data); // we don't care if f->data is null
  f->data = NULL;
}
//////////////////////////////////////////////////////////////

void game_expand_field(field_t *f) {
  int32_t rows = f->rows * 2;
  int32_t cols = f->cols * 2;
  cell_t *data = calloc(rows * cols, sizeof(cell_t));

  for (int32_t r = 0; r < f->rows; ++r) {
    int32_t dst_r = r + f->rows / 2;
    int32_t dst_c = f->cols / 2;
    memcpy(&data[dst_r * cols + dst_c], &f->data[r * f->cols],
           sizeof(cell_t) * f->cols);
  }

  free(f->data);
  f->data = data;
  f->cols = cols;
  f->rows = rows;
}
//////////////////////////////////////////////////////////////

void game_shrink_field(field_t *f) {
  int32_t left, right, top, bottom; // left, right, top, bottom
  left = f->cols;
  right = 0;
  top = f->rows;
  bottom = 0;

  for (int32_t r = 0; r < f->rows; ++r) {
    for (int32_t c = 0; c < f->cols; ++c) {
      if (f->data[r * f->cols + c].b[cell_curr] == cell_dead) {
        continue;
      }

      left = left < c ? left : c;
      right = right > c ? right : c;
      top = top < r ? top : r;
      bottom = bottom > r ? bottom : r;
    }
  }

  int32_t rows = (bottom + 1) - top;
  int32_t cols = (right + 1) - left;
  cell_t *data;

  if (rows * cols == 0) {
    rows = cols = 0; // HACK
    data = NULL;
  } else {
    data = calloc(rows * cols, sizeof(cell_t));
  }

  for (int32_t r = 0; r < rows; ++r) {
    cell_t *ptr_dst_r = &data[r * cols];
    cell_t *ptr_src_r = &f->data[(top + r) * f->cols + left];
    memcpy(ptr_dst_r, ptr_src_r, sizeof(cell_t) * cols);
  }

  game_free(f);
  f->cols = cols;
  f->rows = rows;
  f->data = data;
}
//////////////////////////////////////////////////////////////

void game_calc_cell_status(field_t *f, int32_t row, int32_t col) {
  cell_t *ptr_r = &f->data[row * f->cols];
  int32_t nn = game_not_null_neighbors(f, row, col);
  if (ptr_r[col].b[cell_curr] == cell_alive) {
    ptr_r[col].b[cell_next] = nn < 2 || nn > 3 ? cell_dead : cell_alive;
    return;
  }
  ptr_r[col].b[cell_next] = nn == 3 ? cell_alive : cell_dead;
}
//////////////////////////////////////////////////////////////

void game_iteration(field_t *f) {
  // bounds
  bool need_to_expand = false;
  int rows[2] = {0, f->rows - 1};
  for (int rix = 0; rix < 2; ++rix) {
    cell_t *ptr_r = &f->data[rows[rix] * f->cols];
    for (int32_t c = 0; c < f->cols; ++c) {
      game_calc_cell_status(f, rows[rix], c);
      if (ptr_r[c].b[cell_next] == cell_dead)
        continue;
      need_to_expand = true;
    }
  }

  int cols[2] = {0, f->cols - 1};
  for (int cix = 0; cix < 2; ++cix) {
    for (int r = 0; r < f->rows; ++r) {
      cell_t *ptr_r = &f->data[r * f->cols];
      game_calc_cell_status(f, r, cols[cix]);
      if (ptr_r[cols[cix]].b[cell_next] == cell_dead)
        continue;
      need_to_expand = true;
    }
  }

  // not bound
  for (int32_t r = 1; r < f->rows - 1; ++r) {
    for (int32_t c = 1; c < f->cols - 1; ++c) {
      game_calc_cell_status(f, r, c);
    } // for cols
  } // for rows

  for (int32_t r = 0; r < f->rows; ++r) {
    cell_t *ptr_r = &f->data[r * f->cols];
    for (int32_t c = 0; c < f->cols; ++c) {
      ptr_r[c].b[cell_curr] = ptr_r[c].b[cell_next];
      ptr_r[c].b[cell_next] = cell_dead;
    }
  }

  if (need_to_expand) {
    game_expand_field(f);
  }
}
//////////////////////////////////////////////////////////////

int32_t game_not_null_neighbors(const field_t *field, int32_t row,
                                int32_t col) {
  // dr / dc
  static const int32_t dix[16] = {
      // 8*2
      0,  -1, // left
      -1, -1, // left up
      -1, 0,  // up
      -1, 1,  // right up
      0,  1,  // right
      1,  1,  // right down
      1,  0,  // down
      1,  -1  // left down
  };

  int32_t count = 0;
  for (int32_t i = 0; i < 8; ++i) {
    int32_t dr = row + dix[i * 2];
    int32_t dc = col + dix[i * 2 + 1];

    if (dr < 0 || dr >= field->rows || dc < 0 || dc >= field->cols) {
      continue;
    }
    cell_t *ptr_r = &field->data[dr * field->cols];
    count += ptr_r[dc].b[cell_curr] != cell_dead;
  }
  return count;
}
//////////////////////////////////////////////////////////////

int32_t **arr_to_field(int rows, int cols, int32_t arr[]) {
  int32_t **res = malloc(sizeof(int32_t *) * rows);
  for (int i = 0; i < rows; ++i) {
    res[i] = &arr[i * cols];
  }
  return res;
}
//////////////////////////////////////////////////////////////
