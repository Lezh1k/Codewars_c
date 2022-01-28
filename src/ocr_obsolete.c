#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <math.h>

#include "ocr.h"

#define DEBUG_IMG_SHOW 1
#define IMG_BACKGROUND_COLOR 0xFF
#define IMG_FOREGROUND_COLOR 0x00

// for C11
#ifndef M_2_PI
#define M_2_PI 0.636619772367581343075535053490057448
#endif

#define HIST_LEN 0x100
typedef struct histogram {
  uint32_t total; // total pixels count
  double sum;     // sum of all values in histogram
  uint32_t data[HIST_LEN];
} histogram_t;

typedef struct convolutional_kernel {
  size_t height, width;
  double *mtx;
} convolutional_kernel_t;

typedef struct morphological_kernel {
  size_t height, width;
  uint8_t *mtx;
} morphological_kernel_t;

static double hist_sigma(const histogram_t *hist);
static void img_blur(ocr_image_t *img,
                     const histogram_t *hist) __attribute_used__;


void img_apply_convolution(ocr_image_t *img,
                           const convolutional_kernel_t *kernel) {
  int r, c;
  size_t kr, kc;
  double sum;

  for (r = 0; r < img->height; ++r) {
    for (c = 0; c < img->width; ++c) {
      sum = 0.0;

      for (kr = 0; kr < kernel->height; ++kr) {
        for (kc = 0; kc < kernel->width; ++kc) {
          int ri, ci;
          ri = r + kr - kernel->width / 2;
          ci = c + kc - kernel->height / 2;

          ci = ci < 0 ? 0 : ci;
          ci = ci >= img->width ? (img->width - 1) : ci;
          ri = ri < 0 ? 0 : ri;
          ri = ri >= img->height ? (img->height - 1) : ri;
          sum += img->pixels[ri * img->width + ci] *
              kernel->mtx[kr * kernel->width + kc];
        } // for kr < kernel->height
      }   // for kc < kernel->width

      sum = sum > 255.f ? 255.f : sum;
      sum = sum < 0.f ? 0.f : sum;

      img->pixels[r * img->width + c] = (uint32_t)sum;
    } // for r < img->height
  }   // for c < img->width
}
//////////////////////////////////////////////////////////////

void img_blur(ocr_image_t *img, const histogram_t *hist) {
#define BLUR_N 5
  double sigma = hist_sigma(hist);
  double sigma_2 = sigma * sigma;
  double sum = 0.0;
  convolutional_kernel_t kernel;
  kernel.height = kernel.width = BLUR_N;
  kernel.mtx = malloc(sizeof(double) * BLUR_N * BLUR_N);

  for (size_t r = 0; r < kernel.height; ++r) {
    for (size_t c = 0; c < kernel.width; ++c) {
      int y = (int)r - kernel.height / 2;
      int x = (int)c - kernel.width / 2;

      double pow = -(x * x + y * y) / (2.0 * sigma_2);
      double G = (1.f / (M_2_PI * sigma_2)) * exp(pow);

      sum += G;
      kernel.mtx[r * kernel.width + c] = G;
    }
  }

  // normalize
  for (size_t r = 0; r < kernel.height; ++r) {
    for (size_t c = 0; c < kernel.width; ++c) {
      kernel.mtx[r * kernel.width + c] /= sum;
    }
  }

  img_apply_convolution(img, &kernel);
  free(kernel.mtx);
}
//////////////////////////////////////////////////////////////

double hist_sigma(const histogram_t *hist) {
  double mean, sigma;
  mean = hist->sum / hist->total;
  sigma = 0.0;
  for (int i = 0; i < HIST_LEN; ++i) {
    double t = i - mean;
    sigma += t * t;
  }
  sigma = sqrt(1.0 / hist->total * sigma);
  return sigma;
}
//////////////////////////////////////////////////////////////


#define OCR_SYM_ROWS_N 9
#define OCR_SYM_COLS_N 5
#define OCR_SYM_MTX_SIZE (OCR_SYM_ROWS_N * OCR_SYM_COLS_N)

typedef struct point {
  int32_t x, y;
} point_t;
// region of interest
typedef struct roi {
  uint32_t fg_n; // foreground pixels count;
  point_t top_left;
  point_t bottom_right;
} roi_t;

static uint32_t roi_width(const roi_t *roi) {
  return (uint32_t)(roi->bottom_right.x - roi->top_left.x + 1); // need abs( ?
}

static uint32_t roi_heigth(const roi_t *roi) {
  return (uint32_t)(roi->bottom_right.y - roi->top_left.y + 1); // need abs( ?
}
//////////////////////////////////////////////////////////////

void img_morphological_process_fit(ocr_image_t *img, const roi_t *roi,
                                   const morphological_kernel_t *kernel) {
#define IMG_COLOR_TO_CLEAR 0
  for (int rr = roi->top_left.y; rr <= roi->bottom_right.y; ++rr) {   // roi row
    for (int rc = roi->top_left.x; rc <= roi->bottom_right.x; ++rc) { // roi col

      if (img->pixels[rr * img->width + rc] == IMG_BACKGROUND_COLOR)
        continue; // we don't need to erode background pixels

      bool to_clear = false;
      for (size_t kr = 0; kr < kernel->height && !to_clear; ++kr) { // kernel
        // row
        for (size_t kc = 0; kc < kernel->width && !to_clear;
             ++kc) { // kernel col

          if (kernel->mtx[kr * kernel->width + kc] == 0)
            continue; // we need to compare only ones

          int pr, pc; // point row, point col
          bool is_out_of_bounds = false;

          pr = rr + kr - kernel->width / 2;
          pc = rc + kc - kernel->height / 2;

          is_out_of_bounds = pc < 0 || pc >= img->width || pr < 0 ||
              pr >= img->height || pc < roi->top_left.x ||
              pc > roi->bottom_right.x || pr < roi->top_left.y ||
              pr > roi->bottom_right.y;

          if (is_out_of_bounds) {
            to_clear = true;
            break;
          }

          bool set = img->pixels[pr * img->width + pc] != IMG_BACKGROUND_COLOR;
          to_clear = !set;
        } // for kr < kernel->height
      }   // for kc < kernel->width

      if (!to_clear)
        continue; // we have fit situation here

      img->pixels[rr * img->width + rc] = IMG_COLOR_TO_CLEAR;

    } // for r < roi.height
  }   // for c < roi.width

  for (int rr = roi->top_left.y; rr <= roi->bottom_right.y; ++rr) {
    for (int rc = roi->top_left.x; rc <= roi->bottom_right.x; ++rc) {
      if (img->pixels[rr * img->width + rc] != IMG_COLOR_TO_CLEAR)
        continue;
      img->pixels[rr * img->width + rc] = IMG_BACKGROUND_COLOR;
    } // for r < roi.height
  }   // for c < roi.width
}
//////////////////////////////////////////////////////////////

void img_erode_symbol(ocr_image_t *img, const roi_t *roi) {
  morphological_kernel_t kernel;
  kernel.width = (roi_width(roi) + OCR_SYM_COLS_N - 1) / OCR_SYM_COLS_N;
  kernel.height = (roi_heigth(roi) + OCR_SYM_ROWS_N - 1) / OCR_SYM_ROWS_N;
  if (kernel.width < 2 || kernel.height < 2)
    return;
  kernel.mtx = malloc(sizeof(uint8_t) * kernel.width * kernel.height);
  memset(kernel.mtx, 1, sizeof(uint8_t) * kernel.width * kernel.height);
  img_morphological_process_fit(img, roi, &kernel);
  free(kernel.mtx);
}
//////////////////////////////////////////////////////////////

ocr_image_t img_from_histogram(const histogram_t *hist) {
  ocr_image_t img;
#define WIDTH 4
#define HEIGHT 300

  img.width = HIST_LEN * WIDTH;
  img.height = HEIGHT;
  img.pixels = malloc(sizeof(uint32_t) * img.height * img.width);

  uint32_t max = hist->data[0];
  for (int c = 1; c < HIST_LEN; ++c) {
    max = max < hist->data[c] ? hist->data[c] : max;
  }

  for (int c = 0; c < HIST_LEN; ++c) {
    int hist_val = ((double)hist->data[c] / (double)max) * img.height;
    for (int r = 0; r < hist_val; ++r) {
      for (int cc = 0; cc < WIDTH; ++cc) {
        img.pixels[r * img.width + c * WIDTH + cc] = IMG_FOREGROUND_COLOR;
      }
    }

    for (int r = hist_val; r < img.height; ++r) {
      for (int cc = 0; cc < WIDTH; ++cc) {
        img.pixels[r * img.width + c * WIDTH + cc] = IMG_BACKGROUND_COLOR;
      }
    }
  } // for c < img.width

  return img;
}
//////////////////////////////////////////////////////////////


typedef enum move_dir {
  MD_STAY = 0,
  // base
  MD_UP,
  MD_LEFT,
  MD_RIGHT,
  MD_DOWN,
  // complex
  MD_LEFT_UP,
  MD_RIGHT_UP,
  MD_LEFT_DOWN,
  MD_RIGHT_DOWN,
  //
  MD_LAST
} move_dir_t;


typedef struct roi_mtx {
  uint8_t mtx[OCR_SYM_MTX_SIZE];
} roi_mtx_t;

static void roi_mtx_print(const roi_mtx_t *mtx) {
  printf("\n");
  for (int r = 0; r < OCR_SYM_ROWS_N; ++r) {
    for (int c = 0; c < OCR_SYM_COLS_N; ++c) {
      printf("%d,", mtx->mtx[r * OCR_SYM_COLS_N + c]);
    }
    printf("\n");
  }
}


static void roi_mtx_shift_inplace(roi_mtx_t *src, move_dir_t dir) {
  switch (dir) {
    case MD_STAY:
      return; // do nothing
    case MD_LEFT_UP:
      roi_mtx_shift_inplace(src, MD_LEFT);
      roi_mtx_shift_inplace(src, MD_UP);
      return;
    case MD_LEFT_DOWN:
      roi_mtx_shift_inplace(src, MD_LEFT);
      roi_mtx_shift_inplace(src, MD_DOWN);
      return;
    case MD_RIGHT_UP:
      roi_mtx_shift_inplace(src, MD_RIGHT);
      roi_mtx_shift_inplace(src, MD_UP);
      return;
    case MD_RIGHT_DOWN:
      roi_mtx_shift_inplace(src, MD_RIGHT);
      roi_mtx_shift_inplace(src, MD_DOWN);
      return;
    default:
      break;
  }

  bool to_left = (dir == MD_UP) || (dir == MD_LEFT);
  if (dir == MD_LEFT || dir == MD_RIGHT) {
    for (int r = 0; r < OCR_SYM_ROWS_N; ++r) {
      array_shift(&src->mtx[r * OCR_SYM_COLS_N],
          OCR_SYM_COLS_N, to_left);
    }
  }

  if (dir == MD_UP || dir == MD_DOWN) {
    uint8_t line[OCR_SYM_ROWS_N] = {0};
    for (int c = 0; c < OCR_SYM_COLS_N; ++c) {
      for (int r = 0; r < OCR_SYM_ROWS_N; ++r) {
        line[r] = src->mtx[r * OCR_SYM_COLS_N + c];
      }
      array_shift(line, OCR_SYM_ROWS_N, to_left);
      for (int r = 0; r < OCR_SYM_ROWS_N; ++r) {
        src->mtx[r * OCR_SYM_COLS_N + c] = line[r];
      }
    }
  } // dir == up || dir == down
}
//////////////////////////////////////////////////////////////

roi_mtx_t
img_mtx_from_roi(const ocr_image_t *img,
                 const roi_t *roi) {
  roi_mtx_t res = {0};
  uint32_t rhi = roi_heigth(roi);
  uint32_t cwi = roi_width(roi);
  double row_hf = rhi / (double)OCR_SYM_ROWS_N;
  double col_wf = cwi / (double)OCR_SYM_COLS_N;

  for (uint32_t r = 0; r < OCR_SYM_ROWS_N; ++r) {
    uint32_t rr_start = r * row_hf;
    uint32_t rr_end = (r + 1) * row_hf;
    uint32_t r_size = rr_end - rr_start;

    for (uint32_t c = 0; c < OCR_SYM_COLS_N; ++c) {
      uint32_t cc_start = c * col_wf;
      uint32_t cc_end = (c + 1) * col_wf;
      uint32_t c_size = cc_end - cc_start;

      uint32_t fn = 0; // foreground count

      for (uint32_t rr = rr_start; rr < rr_end; ++rr) {
        for (uint32_t cc = cc_start; cc < cc_end; ++cc) {

          uint32_t rix = roi->top_left.y + rr;
          uint32_t cix = roi->top_left.x + cc;

          uint32_t val = img->pixels[rix * img->width + cix];
          if (val == 0xff)
            continue;
          ++fn;
        }
      }

      double percent = (double)fn / (r_size * c_size);
      res.mtx[r * OCR_SYM_COLS_N + c] = percent >= 0.7 ? 1 : 0;
    } // for cols in ocr_sym_cols
  }   // for rows in ocr_sym_rows
  return res;
}
//////////////////////////////////////////////////////////////
