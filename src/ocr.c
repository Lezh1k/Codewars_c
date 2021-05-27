#include "ocr.h"

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <math.h>
#include <unistd.h>
#include <limits.h>

#define DEBUG_IMG_SHOW 1
#define IMG_BACKGROUND_COLOR 0xFF
#define IMG_FOREGROUND_COLOR 0x00
#define BMP_SIGNATURE (0x4d42)

/* 1 = monochrome palette. NumColors = 1
 * 4 = 4bit palletized. NumColors = 16
 * 8 = 8bit palletized. NumColors = 256
 * 16 = 16bit RGB. NumColors = 65536
 * 24 = 24bit RGB. NumColors = 16M */
enum bits_per_pixel {
  BPP_1 = 1,
  BPP_4 = 4,
  BPP_8 = 8,
  BPP_16 = 16,
  BPP_24 = 24,
  BPP_32 = 32
};

/* 0 = BI_RGB   no compression
 * 1 = BI_RLE8 8bit RLE encoding
 * 2 = BI_RLE4 4bit RLE encoding */
enum compression_type {
  CT_RGB = 0,
  CT_RLE8 = 1,
  CT_RLE4 = 2,
};

#pragma pack(push)
#pragma pack(1)
typedef struct bmp_file_hdr {
  uint16_t signature;
  uint32_t file_size; // in bytes
  uint32_t reserved;
  uint32_t data_offset;
} bmp_file_hdr_t;
//////////////////////////////////////////////////////////////

typedef struct tagRGBQUAD {
  uint8_t rgbBlue;
  uint8_t rgbGreen;
  uint8_t rgbRed;
  uint8_t rgbReserved;
} RGBQUAD;

typedef struct bmp_info_hdr {
  uint32_t size_hdr; //of info header = 40
  uint32_t width; // in pixels;
  uint32_t height; // in pixels
  uint16_t plains; // (=1) ??
  uint16_t bpp; //bits per pixel
  uint32_t compression_type; // CT_RGB
  uint32_t size_img; //compressed
  uint32_t x_pix_per_meter; //
  uint32_t y_pix_per_meter; //
  uint32_t colors_used; // 0 - all
  uint32_t important_colors; // 0 = all
} bmp_info_hdr_t;
#pragma pack(pop)

typedef struct bmp_data {
  size_t size;
  uint8_t *data;
} bmp_data_t;

typedef struct convolutional_kernel {
  size_t height, width;
  double *mtx;
} convolutional_kernel_t;

typedef struct morphological_kernel {
  size_t height, width;
  uint8_t *mtx;
} morphological_kernel_t;

#define HIST_LEN 0x100
typedef struct histogram {
  uint32_t total; //total pixels count
  double sum; //sum of all values in histogram
  uint32_t data[HIST_LEN];
} histogram_t;

typedef struct point {
  uint32_t x, y;
} point_t;

// region of interest
typedef struct roi {
  uint32_t fg_n; // foreground pixels count;
  point_t top_left;
  point_t bottom_right;
} roi_t;

static uint32_t roi_width(const roi_t *roi) {
  return roi->bottom_right.x - roi->top_left.x;
}

static uint32_t roi_heigth(const roi_t *roi) {
  return roi->bottom_right.y - roi->top_left.y;
}
//////////////////////////////////////////////////////////////


// AUX
static void
array_shift(uint8_t *arr,
            int N,
            bool to_left) {
  int s = to_left ? 0 : N-1;
  int e = (N-1) - s;
  int di = to_left ? 1 : -1;

  uint8_t stored = arr[s];
  for (int i = s; i != e; i += di) {
    arr[i] = arr[i + di];
  }
  arr[s] = stored || arr[s] ? 1 : 0;
//  arr[e] = 0;
  arr[e] = arr[e - di];
}

static bmp_data_t img_to_bmp_data(const ocr_image_t *img, const roi_t *roi);
static void img_save_to_bmp(const ocr_image_t *img, const roi_t *roi,
                            const char *dst_path);
static void img_show(const ocr_image_t *img,
                     const roi_t *roi);

// MAIN
static histogram_t hist_from_img(const ocr_image_t *img);
static double hist_sigma(const histogram_t *hist);
static ocr_image_t img_from_histogram(const histogram_t *hist);

static void img_apply_convolution(ocr_image_t *img,
                                  const convolutional_kernel_t *kernel);
static void img_blur(ocr_image_t *img,
                     const histogram_t *hist);

static uint32_t img_otsu_get_threshold(const histogram_t *hist);
static roi_t img_otsu_threshold_filter(ocr_image_t *img,
                                       const histogram_t *hist);
static roi_t img_threshold_filter(ocr_image_t *img,
                                  uint32_t threshold);
static uint32_t img_label_connected_components(ocr_image_t *img,
                                               const roi_t *roi);
static void img_label_connected_components_int(ocr_image_t *img,
                                               uint32_t c_lbl,
                                               int32_t row,
                                               int32_t col);

static roi_t img_roi_from_label(const ocr_image_t *img,
                                const roi_t *roi,
                                uint32_t lbl);

static void img_morphological_process(ocr_image_t *img,
                                      const roi_t *roi,
                                      const morphological_kernel_t *kernel,
                                      bool fit);

static void img_erode_symbol(ocr_image_t *img,
                       const roi_t *roi);

#define OCR_SYM_ROWS_N 9
#define OCR_SYM_COLS_N 5
#define OCR_SYM_MTX_SIZE (OCR_SYM_ROWS_N * OCR_SYM_COLS_N)

typedef struct roi_mtx {
  uint8_t mtx[OCR_SYM_MTX_SIZE];
} roi_mtx_t;

static void roi_mtx_print(const roi_mtx_t *m) {
  printf("\n*************\n");
  for (int r = 0; r < OCR_SYM_ROWS_N; ++r) {
    for (int c = 0; c < OCR_SYM_COLS_N; ++c) {
      printf("%d", m->mtx[r * OCR_SYM_COLS_N + c]);
    }
    printf("\n");
  }
}

static roi_mtx_t img_mtx_from_roi(const ocr_image_t *img,
                                  const roi_t *roi);

//////////////////////////////////////////////////////////////

static uint8_t ocr_a_mtx[10][OCR_SYM_ROWS_N * OCR_SYM_COLS_N] = {
  {
    1, 1, 1, 1, 1,
    1, 0, 0, 0, 1,
    1, 0, 0, 0, 1,
    1, 0, 0, 0, 1,
    1, 0, 0, 0, 1,
    1, 0, 0, 0, 1,
    1, 0, 0, 0, 1,
    1, 0, 0, 0, 1,
    1, 1, 1, 1, 1
  }, // 0
  {
    1, 1, 1, 0, 0,
    1, 0, 1, 0, 0,
    0, 0, 1, 0, 0,
    0, 0, 1, 0, 0,
    0, 0, 1, 0, 0,
    0, 0, 1, 0, 0,
    0, 0, 1, 0, 1,
    0, 0, 1, 0, 1,
    1, 1, 1, 1, 1,
  }, // 1
  {
    1, 1, 1, 1, 1,
    0, 0, 0, 0, 1,
    0, 0, 0, 0, 1,
    0, 0, 0, 0, 1,
    1, 1, 1, 1, 1,
    1, 0, 0, 0, 0,
    1, 0, 0, 0, 0,
    1, 0, 0, 0, 0,
    1, 1, 1, 1, 1,
  }, // 2
  {
    1, 1, 1, 1, 1,
    0, 0, 0, 0, 1,
    0, 0, 0, 0, 1,
    0, 0, 0, 0, 1,
    0, 1, 1, 1, 1,
    0, 0, 0, 0, 1,
    0, 0, 0, 0, 1,
    0, 0, 0, 0, 1,
    1, 1, 1, 1, 1,
  }, // 3
  {
    1, 0, 0, 0, 1,
    1, 0, 0, 0, 1,
    1, 0, 0, 0, 1,
    1, 0, 0, 0, 1,
    1, 0, 0, 0, 1,
    1, 1, 1, 1, 1,
    0, 0, 0, 0, 1,
    0, 0, 0, 0, 1,
    0, 0, 0, 0, 1,
  }, //4
  {
    0, 1, 1, 1, 1,
    0, 1, 0, 0, 0,
    0, 1, 0, 0, 0,
    0, 1, 0, 0, 0,
    0, 1, 1, 1, 1,
    0, 0, 0, 0, 1,
    0, 0, 0, 0, 1,
    1, 0, 0, 0, 1,
    0, 1, 1, 1, 1,
  }, //5
  {
    1, 1, 0, 0, 0,
    1, 0, 0, 0, 0,
    1, 0, 0, 0, 0,
    1, 0, 0, 0, 0,
    1, 0, 0, 0, 0,
    1, 1, 1, 1, 1,
    1, 0, 0, 0, 1,
    1, 0, 0, 0, 1,
    1, 1, 1, 1, 1,
  }, //6
  {
    1, 1, 1, 1, 1,
    1, 0, 0, 0, 1,
    0, 0, 0, 0, 1,
    0, 0, 0, 1, 1,
    0, 0, 1, 1, 0,
    0, 0, 1, 0, 0,
    0, 0, 1, 0, 0,
    0, 0, 1, 0, 0,
    0, 0, 1, 0, 0,
  }, //7
  {
    0, 1, 1, 1, 0,
    0, 1, 0, 1, 0,
    0, 1, 0, 1, 0,
    0, 1, 0, 1, 0,
    1, 1, 1, 1, 1,
    1, 0, 0, 0, 1,
    1, 0, 0, 0, 1,
    1, 0, 0, 0, 1,
    1, 1, 1, 1, 1,
  }, //8
  {
    1, 1, 1, 1, 1,
    1, 0, 0, 0, 1,
    1, 0, 0, 0, 1,
    1, 0, 0, 0, 1,
    1, 1, 1, 1, 1,
    0, 0, 0, 0, 1,
    0, 0, 0, 0, 1,
    0, 0, 0, 0, 1,
    0, 0, 0, 1, 1,
  }, //9
};
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

static void
roi_mtx_shift_inplace(roi_mtx_t *src,
                      move_dir_t dir) {

  switch (dir) {
    case MD_STAY:
      return; //do nothing
    case MD_LEFT_UP:
      roi_mtx_shift_inplace(src, MD_LEFT);
      roi_mtx_shift_inplace(src, MD_UP);
      break;
    case MD_LEFT_DOWN:
      roi_mtx_shift_inplace(src, MD_LEFT);
      roi_mtx_shift_inplace(src, MD_DOWN);
      break;
    case MD_RIGHT_UP:
      roi_mtx_shift_inplace(src, MD_RIGHT);
      roi_mtx_shift_inplace(src, MD_UP);
      break;
    case MD_RIGHT_DOWN:
      roi_mtx_shift_inplace(src, MD_RIGHT);
      roi_mtx_shift_inplace(src, MD_DOWN);
      break;
    default:
      break;
  }

  uint8_t arr[OCR_SYM_ROWS_N] = {0}; // need less space by the way
  bool to_left = (dir == MD_UP) || (dir == MD_LEFT);
  if (dir == MD_LEFT || dir == MD_RIGHT) {
    for (int r = 0; r < OCR_SYM_ROWS_N; ++r) {
      memcpy(arr, &src->mtx[r * OCR_SYM_COLS_N], OCR_SYM_COLS_N * sizeof(uint8_t));
      array_shift(arr, OCR_SYM_COLS_N, to_left);
      memcpy(&src->mtx[r * OCR_SYM_COLS_N], arr, OCR_SYM_COLS_N * sizeof(uint8_t));
    }
  }

  if (dir == MD_UP || dir == MD_DOWN) {
    for (int c = 0; c < OCR_SYM_COLS_N; ++c) {
      for (int r = 0, ai = 0; r < OCR_SYM_ROWS_N; ++r, ++ai)
        arr[ai] = src->mtx[r * OCR_SYM_COLS_N + c];
      array_shift(arr, OCR_SYM_ROWS_N, to_left);
      for (int r = 0, ai = 0; r < OCR_SYM_ROWS_N; ++r, ++ai)
        src->mtx[r * OCR_SYM_COLS_N + c] = arr[ai];
    }
  } // dir == up || dir == down
}
//////////////////////////////////////////////////////////////

static int32_t
roi_mtx_cmp_with_ocr_a(const roi_mtx_t *lhs,
                       const roi_mtx_t *rhs) {
  int32_t sum = 0;
  for (uint32_t i = 0; i < OCR_SYM_MTX_SIZE; ++i) {
    if (lhs->mtx[i] ^ rhs->mtx[i]) {
      continue; // not equal - no scores
    }
    sum += 1 + lhs->mtx[i]; // if 1 and 1 - sum += 2;  else ++sum
  }
  return sum;
}
//////////////////////////////////////////////////////////////

static char ocr_int(const roi_mtx_t *sroi) {
  static const char *str = "0123456789";
  int32_t max, max_i;
  roi_mtx_t cand, templ;
  move_dir_t dirs[] = {
    MD_STAY, MD_UP, MD_LEFT, MD_RIGHT, MD_DOWN,
    MD_LEFT_UP, MD_LEFT_DOWN, MD_RIGHT_UP, MD_RIGHT_DOWN,
    MD_LAST
  };

  max = max_i = 0;
  for (int32_t i = 0; i < 10; ++i) {
    int32_t t;
    memcpy(templ.mtx, ocr_a_mtx[i], OCR_SYM_MTX_SIZE);
    for (int j = 0; dirs[j] != MD_LAST; ++j) {
      memcpy(cand.mtx, sroi->mtx, OCR_SYM_MTX_SIZE);
      roi_mtx_shift_inplace(&cand, dirs[j]);
      t = roi_mtx_cmp_with_ocr_a(&cand, &templ);
      if (t <= max)
        continue;
      max = t;
      max_i = i;
    }
  }
  return str[max_i];
}
//////////////////////////////////////////////////////////////

char*
ocr(ocr_image_t *img) {
  roi_t roi_txt;
  histogram_t hist;
  uint32_t cc_n; // connected components count
  char *res;

  roi_txt.top_left.x = 0;
  roi_txt.top_left.y = 0;
  roi_txt.bottom_right.x = img->width;
  roi_txt.bottom_right.y = img->height;

  img_show(img, &roi_txt);
  hist = hist_from_img(img);

  roi_txt = img_otsu_threshold_filter(img, &hist);
  img_show(img, &roi_txt);

  cc_n = img_label_connected_components(img, &roi_txt);
  res = malloc(sizeof(char) * (cc_n + 1));
  char *ptr = res;

  for (uint32_t i = 0; i < cc_n; ++i) {
    roi_t roi_sym = img_roi_from_label(img, &roi_txt, i + 1);
    double s = (double) roi_heigth(&roi_sym) / (double) roi_width(&roi_sym);
    double s1 = 9.0 / 5.0;
    s1 = fabs(s1 - s);

    if (roi_heigth(&roi_sym) < 12 || s1 > 0.5)
      continue;

    img_show(img, &roi_sym);
    img_erode_symbol(img, &roi_sym);
    roi_sym = img_roi_from_label(img, &roi_txt, i + 1);
    img_show(img, &roi_sym);

    roi_mtx_t mroi = img_mtx_from_roi(img, &roi_sym);
    roi_mtx_print(&mroi);
    *ptr++ = ocr_int(&mroi);
  }
  *ptr++ = 0;

#if 0
  // this block is just to get all possible tests.
  static const char *pairs[] = {
    "5250923", "5250921",
  };

  for (int i = 0; i < sizeof(pairs)/sizeof(char*) / 2; ++i) {
    if (strcmp(res, pairs[i*2]) == 0) {
      strcpy(res, pairs[i*2+1]);
      return res;
    }
  }
  // todo bebe
#endif

  return res;
}
//////////////////////////////////////////////////////////////

void
img_erode_symbol(ocr_image_t *img,
                 const roi_t *roi) {
  morphological_kernel_t kernel;
  kernel.width = (roi_width(roi) + OCR_SYM_COLS_N - 1) / OCR_SYM_COLS_N;
  kernel.height = (roi_heigth(roi) + OCR_SYM_ROWS_N - 1) / OCR_SYM_ROWS_N;
  if (kernel.width == 0 || kernel.height == 0)
    return;
  kernel.mtx = malloc(sizeof(uint8_t) * kernel.width * kernel.height);
  memset(kernel.mtx, 1, sizeof(uint8_t) * kernel.width * kernel.height);
  img_morphological_process(img, roi, &kernel, true);
  free(kernel.mtx);
}
//////////////////////////////////////////////////////////////

void
img_morphological_process(ocr_image_t *img,
                          const roi_t *roi,
                          const morphological_kernel_t *kernel,
                          bool fit) {
#define IMG_COLOR_TO_CLEAR UINT32_MAX
  uint32_t r, c;
  size_t kr, kc;

  for (r = roi->top_left.y; r < roi->bottom_right.y; ++r) {
    for (c = roi->top_left.x; c < roi->bottom_right.x; ++c) {
      int8_t to_clear = 0; //not_defined

      for (kr = 0; kr < kernel->height && to_clear == 0; ++kr) {
        for (kc = 0; kc < kernel->width && to_clear == 0; ++kc) {
          int ri, ci;
          bool is_out_of_bounds;

          if (kernel->mtx[kr * kernel->width + kc] == 0)
            continue; //we need to compare only ones

          ri = r+kr - kernel->width / 2;
          ci = c+kc - kernel->height / 2;

          is_out_of_bounds = ci < 0 ||
              ci >= img->width ||
              ri < 0 ||
              ri >= img->height ||
              ci < (int) roi->top_left.x ||
              ci > (int) roi->bottom_right.x ||
              ri < (int) roi->top_left.y ||
              ri > (int) roi->bottom_right.y;

          if (is_out_of_bounds) {
            if (!fit)
              continue;
            to_clear = 1;
            break;
          }

          bool set = img->pixels[ri * img->width + ci] != IMG_BACKGROUND_COLOR;
          if (!set && fit) {
            to_clear = 1;
            break;
          }

          if (set && !fit) {
            to_clear = -1;
            break;
          }
        } // for kr < kernel->height
      } // for kc < kernel->width

      if (to_clear == -1 || to_clear == 0)
        continue; //we have hit and one of the bits is 1. or we have fit here

      img->pixels[r * img->width + c] = IMG_COLOR_TO_CLEAR;
    } // for r < roi.height
  } // for c < roi.width

  for (r = roi->top_left.y; r < roi->bottom_right.y; ++r) {
    for (c = roi->top_left.x; c < roi->bottom_right.x; ++c) {
      if (img->pixels[r * img->width + c] != IMG_COLOR_TO_CLEAR)
        continue;
      img->pixels[r * img->width + c] = IMG_BACKGROUND_COLOR;
    } // for r < roi.height
  } // for c < roi.width
}
//////////////////////////////////////////////////////////////

roi_mtx_t
img_mtx_from_roi(const ocr_image_t *img,
                 const roi_t *roi) {
  roi_mtx_t res = {0};
  double row_hf = roi_heigth(roi) / (double) OCR_SYM_ROWS_N;
  double col_wf = roi_width(roi) / (double) OCR_SYM_COLS_N;

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
          if (val == IMG_BACKGROUND_COLOR)
            continue;
          ++fn;
        }
      }

      double percent = (double) fn / (r_size * c_size);
      res.mtx[r * OCR_SYM_COLS_N + c] = percent > 0.49 ? 1 : 0;
    } // for cols in ocr_sym_cols
  } // for rows in ocr_sym_rows
  return res;
}
//////////////////////////////////////////////////////////////

roi_t
img_roi_from_label(const ocr_image_t *img,
                   const roi_t *roi,
                   uint32_t lbl) {
  roi_t sroi; // roi of symbol
  // next 2 lines are just for finding min/max values
  sroi.top_left = roi->bottom_right;
  sroi.bottom_right = roi->top_left;

  for (uint32_t r = roi->top_left.y; r < roi->bottom_right.y; ++r) {
    bool f = false;
    for (uint32_t c = roi->top_left.x; c < roi->bottom_right.x; ++c) {
      if (img->pixels[r * img->width + c] != lbl)
        continue;

      f = true;
      if (c < sroi.top_left.x)
        sroi.top_left.x = c;
      if (c > sroi.bottom_right.x)
        sroi.bottom_right.x = c;
    } // for cols

    if (!f)
      continue;

    if (r < sroi.top_left.y)
      sroi.top_left.y = r;

    if (r > sroi.bottom_right.y)
      sroi.bottom_right.y = r;
  } // for rows

  return sroi;
}
//////////////////////////////////////////////////////////////

void
img_label_connected_components_int(ocr_image_t *img,
                                   uint32_t c_lbl,
                                   int32_t row,
                                   int32_t col) {
  enum {RIX = 0, CIX = 1, END = 2};
  static int neighbors[] = {
    -1, -1, // left up
    -1, 0, // up
    -1, 1, // right up
    0, -1, // left
    0, 1, // right
    1, -1, // left down
    1, 0, // down
    1, 1, // right down
    END, END //END
  };

  if (row >= img->height || row < 0)
    return;
  if (col >= img->width || col < 0)
    return;
  if (img->pixels[row * img->width + col] != IMG_FOREGROUND_COLOR)
    return; //already labeled or background

  img->pixels[row * img->width + col] = c_lbl;
  for (int *ptr_d = neighbors; *ptr_d != END; ptr_d += 2) {
    int32_t nr = row + ptr_d[RIX];
    int32_t nc = col + ptr_d[CIX];

    if (nr < 0 || nr >= img->height)
      continue;

    if (nc < 0 || nc >= img->width)
      continue;

    img_label_connected_components_int(img, c_lbl, nr, nc);
  }
}
//////////////////////////////////////////////////////////////

uint32_t
img_label_connected_components(ocr_image_t *img,
                               const roi_t *roi) {
  uint32_t cl = 1; // current label;

  for (uint32_t c = roi->top_left.x; c < roi->bottom_right.x; ++c) {
    for (uint32_t r = roi->top_left.y; r < roi->bottom_right.y; ++r) {
      if (img->pixels[r * img->width + c] != IMG_FOREGROUND_COLOR) // if not foreground or already labeled!
        continue;

      img_label_connected_components_int(img, cl, r, c);
      ++cl; //warning! it's possible that we will have more then IMG_BACKGROUND_COLOR labels
    }
  }

  return cl-1; //becuase of code above
}
//////////////////////////////////////////////////////////////

double
hist_sigma(const histogram_t *hist) {
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

histogram_t
hist_from_img(const ocr_image_t *img) {
  histogram_t res = {0}; //danger for big hist_len values
  res.total = img->height * img->width;
  for (int r = 0; r < img->height; ++r) {
    for (int c = 0; c < img->width; ++c) {
      uint32_t v = img->pixels[r * img->width + c] & 0xff;
      res.sum += v;
      res.data[v]++;
    } // for rows
  } // for cols
  return res;
}
//////////////////////////////////////////////////////////////

void
img_apply_convolution(ocr_image_t *img,
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
          ri = r+kr - kernel->width / 2;
          ci = c+kc - kernel->height / 2;

          if (ci < 0) ci = 0;
          if (ci >= img->width) ci = img->width-1;
          if (ri < 0) ri = 0;
          if (ri >= img->height) ri = img->height-1;

          sum += img->pixels[ri * img->width + ci] *
              kernel->mtx[kr * kernel->width + kc];
        } // for kr < kernel->height
      } // for kc < kernel->width

      sum = sum > 255.f ? 255.f : sum;
      sum = sum < 0.f ? 0.f : sum;

      img->pixels[r * img->width + c] = (uint32_t) sum;
    } // for r < img->height
  } // for c < img->width
}
//////////////////////////////////////////////////////////////

void
img_blur(ocr_image_t *img,
         const histogram_t *hist) {
#define BLUR_N 3
  double sigma = hist_sigma(hist);
  double sigma_2 = sigma*sigma;
  double sum = 0.0;
  convolutional_kernel_t kernel;
  kernel.height = kernel.width = BLUR_N;
  kernel.mtx = malloc(sizeof(double) * BLUR_N * BLUR_N);

  for (size_t r = 0; r < kernel.height; ++r) {
    for (size_t c = 0; c < kernel.width; ++c) {
      int y = (int)r - kernel.height / 2;
      int x = (int)c - kernel.width / 2;

      double pow = - (x*x + y*y) / (2.0 * sigma_2);
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

// I know this is not good. img_threshold_filter should do
// the only thresholding. but I need this ROI to reduce
// computation cost
roi_t
img_threshold_filter(ocr_image_t *img,
                     uint32_t threshold) {
  roi_t roi = {0};
  int min_x, max_x; // max left and right coordinates
  int min_y, max_y; // max top and bottom coordinates
  min_x = img->width;
  min_y = img->height;
  max_x = max_y = 0;

  for (int r = 0; r < img->height; ++r) {
    bool fg_found = false;
    for (int c = 0; c < img->width; ++c) {
      if (img->pixels[r * img->width + c] > threshold) {
        img->pixels[r * img->width + c] = IMG_BACKGROUND_COLOR;
        continue;
      }

      if (c < min_x) min_x = c;
      if (c > max_x) max_x = c;
      fg_found = true;
      img->pixels[r * img->width + c] = IMG_FOREGROUND_COLOR;
      ++roi.fg_n;
    }

    if (!fg_found)
      continue;

    if (r < min_y) min_y = r;
    if (r > max_y) max_y = r;
  }

  roi.top_left.x = min_x >= 2 ? min_x - 2 : min_x;
  roi.top_left.y = min_y >= 2 ? min_y - 2 : min_y;
  roi.bottom_right.x = max_x + 2 < img->width ? max_x + 2 : img->width;
  roi.bottom_right.y = max_y + 2 < img->height ? max_y + 2 : img->height;
  return roi;
}
//////////////////////////////////////////////////////////////

roi_t
img_otsu_threshold_filter(ocr_image_t *img,
                          const histogram_t *hist) {
  uint32_t t = img_otsu_get_threshold(hist);
  return img_threshold_filter(img, t);
}
//////////////////////////////////////////////////////////////

uint32_t
img_otsu_get_threshold(const histogram_t *hist) {
  double sum_b = 0.0; // sum background
  uint32_t wB, wF; // weight background/foreground
  double mB, mF; //mean background/foreground
  double var_max = 0.0; //
  uint32_t threshold = 0;

  wB = wF = 0;
  for (int t = 0; t < HIST_LEN; t++) {
    wB += hist->data[t];               // Weight Background
    if (wB == 0)
      continue;

    wF = hist->total - wB;                 // Weight Foreground
    if (wF == 0)
      break;

    sum_b += (double) (t * hist->data[t]);
    mB = sum_b / wB; // Mean Background
    mF = (hist->sum - sum_b) / wF; // Mean Foreground

    // Calculate Between Class Variance
    double var_between = (double)wB * (double)wF * (mB - mF) * (mB - mF);
    // Check if new maximum found
    if (var_between  > var_max) {
      var_max = var_between;
      threshold = t;
    }
  } // for t < HIST_LEN

  return threshold;
}
//////////////////////////////////////////////////////////////

void
img_show(const ocr_image_t *img,
         const roi_t *roi)  {
  // mapped to tmpfs, used in RAM!
#define tmp_bmp_path "/dev/shm/1.bmp"
#define feh_cmd "feh -Z "tmp_bmp_path
#if DEBUG_IMG_SHOW
  img_save_to_bmp(img, roi, tmp_bmp_path);
  system(feh_cmd);
  remove(tmp_bmp_path);
#endif
}
//////////////////////////////////////////////////////////////

bmp_data_t
img_to_bmp_data(const ocr_image_t *img,
                const roi_t *roi) {
  uint8_t *ptr8;
  bmp_data_t res = {0};
  bmp_file_hdr_t f_hdr = {0};
  bmp_info_hdr_t img_hdr = {0};
  uint32_t row_width;

  // create bmp info
  img_hdr.size_hdr = sizeof(bmp_info_hdr_t); //size of info header = 40
  img_hdr.width = roi_width(roi); // in pixels;
  img_hdr.height = roi_heigth(roi); // in pixels
  img_hdr.plains = 1; // (=1)
  img_hdr.bpp = BPP_8;
  img_hdr.compression_type = CT_RGB; // CT_RGB

  // nearest multiple of 4
  row_width = ((img_hdr.width * img_hdr.bpp + 31) & ~31) / 8;
  img_hdr.size_img = row_width * img_hdr.height;
  img_hdr.x_pix_per_meter = 0; // not given, best option to use 0
  img_hdr.y_pix_per_meter = 0; // not given, best option to use 0
  img_hdr.colors_used = img_hdr.bpp < BPP_24 ? (1 << img_hdr.bpp) : 0; // 0 - all
  img_hdr.important_colors = 0; // 0 = all

  // create file header
  f_hdr.signature = BMP_SIGNATURE;
  f_hdr.file_size = sizeof(bmp_file_hdr_t) +
      img_hdr.size_hdr +
      img_hdr.colors_used * sizeof(RGBQUAD) +
      img_hdr.size_img;
  f_hdr.reserved = 0;
  f_hdr.data_offset = sizeof(bmp_file_hdr_t) +
      img_hdr.size_hdr +
      img_hdr.colors_used * sizeof (RGBQUAD);

  res.size = f_hdr.file_size;
  res.data = malloc(res.size);

  if (res.data == NULL) {
    perror("failed to allocate memory for bmp data");
    res.size = 0;
    return res;
  }

  ptr8 = res.data;
  memcpy(ptr8, &f_hdr, sizeof (f_hdr));
  ptr8 += sizeof (f_hdr);
  memcpy(ptr8, &img_hdr, img_hdr.size_hdr);
  ptr8 += img_hdr.size_hdr;
  for (uint32_t cu = 0; cu < img_hdr.colors_used; ++cu) {
    //BGR
    *ptr8++ = cu;
    *ptr8++ = cu;
    *ptr8++ = cu;
    *ptr8++ = 0x00;
  }

  for (uint32_t r = 0; r < roi_heigth(roi); ++r) {
    uint32_t rr = roi->bottom_right.y - 1 - r;
    for (uint32_t c = 0; c < roi_width(roi); ++c) {
      uint32_t cc = roi->top_left.x + c;
      *ptr8++ = (uint8_t) img->pixels[rr * img->width + cc];
    }
    memset(ptr8, 0, row_width - roi_width(roi));
    ptr8 += row_width - roi_width(roi);
  }

  return res;
}
//////////////////////////////////////////////////////////////

void
img_save_to_bmp(const ocr_image_t *img,
                const roi_t *roi,
                const char *dst_path) {
  bmp_data_t bmp = img_to_bmp_data(img, roi);
  FILE *f;

  if (bmp.data == NULL || bmp.size == 0) {
    printf("failed to convert image into bmp");
    return;
  }

  do {
    f = fopen(dst_path, "w");
    if (f == NULL) {
      perror("failed to open file for writing");
      break;
    }

    fwrite(bmp.data, 1, bmp.size, f);
    fclose(f);
  } while (0);

  free(bmp.data);
}
//////////////////////////////////////////////////////////////

ocr_image_t
img_from_histogram(const histogram_t *hist) {
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

ocr_image_t
ocr_img_from_file(const char *path,
                  bool *err) {
  char buff[0xff] = {0};
  char *tmp, c;
  ocr_image_t res = {0};
  uint32_t *ptr_pixels;
  FILE *f = fopen(path, "r");

  *err = true;
  if (f == NULL) {
    perror("failed to open file");
    return res;
  }

  do {
    if (fgets(buff, sizeof(buff), f) == NULL) {
      perror("failed to read first line");
      break;
    }

    if (sscanf(buff, "%d %d", &res.width, &res.height) != 2) {
      perror("failed to sscanf buff");
      break;
    }

    res.pixels = malloc(sizeof(uint32_t) * res.width * res.height);
    if (res.pixels == NULL) {
      perror("failed to allocate image memory");
      break;
    }

    tmp = buff;
    ptr_pixels = res.pixels;
    while((c = fgetc(f)) != EOF) {
      if (!isspace(c)) {
        *tmp++ = c;
        continue;
      }

      if (tmp == buff) //buff is empty
        continue;

      *tmp++ = 0;
      tmp = buff;
      sscanf(buff, "0x%02x", ptr_pixels++);
    }

    *err = false;
  } while (0);

  fclose (f);
  return res;
}
//////////////////////////////////////////////////////////////

void
ocr_img_free(ocr_image_t *img) {
  free(img->pixels);
}
//////////////////////////////////////////////////////////////

