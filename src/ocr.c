#include "bmp.h"
#include "ocr.h"

#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DEBUG_IMG_SHOW 0
#define IMG_COLOR_BACKGROUND 0xFF
#define IMG_COLOR_FOREGROUND 0x00

#define OCR_SYM_ROWS_N 9
#define OCR_SYM_COLS_N 5
#define OCR_SYM_MTX_SIZE (OCR_SYM_ROWS_N * OCR_SYM_COLS_N)

#define HIST_LEN 0x100
typedef struct histogram {
  uint32_t total; // total pixels count
  double sum;     // sum of all values in histogram
  uint32_t data[HIST_LEN];
} histogram_t;

typedef struct point {
  int32_t x, y;
} point_t;

// region of interest
typedef struct roi {
  uint32_t fg_n; // foreground pixels count;
  point_t top_left;
  point_t bottom_right;
} roi_t;

static inline uint32_t roi_width(const roi_t *roi) {
  return (uint32_t)(roi->bottom_right.x - roi->top_left.x + 1); // need abs( ?
}

static inline uint32_t roi_heigth(const roi_t *roi) {
  return (uint32_t)(roi->bottom_right.y - roi->top_left.y + 1); // need abs( ?
}

static void roi_img_print_data(const ocr_image_t *img,
                               const roi_t *roi) {
  printf("\n");
  for (uint32_t r = 0; r < roi_heigth(roi); ++r) {
    uint32_t rr = roi->top_left.y + r;
    for (uint32_t c = 0; c < roi_width(roi); ++c) {
      uint32_t cc = roi->top_left.x + c;
      printf("%d,", img->pixels[rr * img->width + cc] == IMG_COLOR_BACKGROUND ? 0 : 1);
    }
    printf("\n");
  }
}

static bmp_data_t img_to_bmp_data(const ocr_image_t *img,
                                  const roi_t *roi);
static void img_save_to_bmp(const ocr_image_t *img,
                            const roi_t *roi,
                            const char *dst_path);
static void img_show(const ocr_image_t *img,
                     const roi_t *roi);

// MAIN
static histogram_t hist_from_img(const ocr_image_t *img);
static roi_t img_threshold_filter(ocr_image_t *img,
                                  uint32_t threshold);
static uint32_t img_otsu_get_threshold(const histogram_t *hist);
static roi_t img_otsu_threshold_filter(ocr_image_t *img,
                                       const histogram_t *hist);

static uint32_t img_label_connected_components(ocr_image_t *img,
                                               const roi_t *roi);
static void img_label_connected_components_int(ocr_image_t *img,
                                               uint32_t c_lbl,
                                               int32_t row,
                                               int32_t col);

static roi_t img_roi_from_label(const ocr_image_t *img,
                                const roi_t *roi,
                                uint32_t lbl);

static roi_t img_roi_rotate(ocr_image_t *img,
                            const roi_t *roi,
                            double angle_degrees);

static char ocr_int(const ocr_image_t *img,
                    const roi_t *roi_sym);

typedef uint32_t ocr_a_mtx_t[OCR_SYM_ROWS_N * OCR_SYM_COLS_N];
static ocr_image_t img_scale(const ocr_image_t img,
                             int width,
                             int height,
                             bool templ);


static ocr_a_mtx_t ocr_a[10] = {
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
    1, 0, 0, 0, 0,
    1, 0, 0, 1, 0,
    1, 0, 0, 1, 0,
    1, 0, 0, 1, 0,
    1, 0, 0, 1, 0,
    1, 1, 1, 1, 1,
    0, 0, 0, 1, 0,
    0, 0, 0, 1, 0,
    0, 0, 0, 1, 0,
  }, // 4
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
  }, // 5
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
  }, // 6
  {
    1, 1, 1, 1, 1,
    1, 0, 0, 0, 1,
    0, 0, 0, 0, 1,
    0, 0, 0, 1, 0,
    0, 0, 1, 0, 0,
    0, 0, 1, 0, 0,
    0, 0, 1, 0, 0,
    0, 0, 1, 0, 0,
    0, 0, 1, 0, 0,
  }, // 7
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
  }, // 8
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
  }, // 9
};
//////////////////////////////////////////////////////////////

ocr_image_t img_scale(const ocr_image_t img,
                      int width,
                      int height,
                      bool templ) {
  ocr_image_t res = {.width = width, .height = height};
  res.pixels = calloc(sizeof(*res.pixels), res.width * res.height);

  double scale_c = (double)res.width / img.width;
  double scale_r = (double)res.height / img.height;

  for (int r = 0; r < res.height; ++r) {
    int nearest_r = floor(r / scale_r);
    if (nearest_r >= img.height)
      continue;

    for (int c = 0; c < res.width; ++c) {

      int nearest_c = floor(c / scale_c);
      int pixel = img.pixels[nearest_r * img.width + nearest_c];
      if (nearest_c >= img.width)
        pixel = img.pixels[nearest_r * img.width + nearest_c - 1];

      if (templ) {
        pixel = pixel == 0 ? IMG_COLOR_BACKGROUND: IMG_COLOR_FOREGROUND;
      }

      res.pixels[r * res.width + c] = pixel;
    }
  }
  return res;
}
//////////////////////////////////////////////////////////////

static int ocr_img_cmp(const ocr_image_t *img1,
                       const roi_t *roi1,
                       const ocr_image_t *img2,
                       const roi_t *roi2) {
  int roi1_w = roi_width(roi1);
  int roi2_w = roi_width(roi2);
  int roi1_h = roi_heigth(roi1);
  int roi2_h = roi_heigth(roi2);

  // todo scale instead of assertion
  assert(roi1_w == roi2_w);
  assert(roi1_h == roi2_h);

  int sum = 0;
  for (int r = 0; r < roi1_h; ++r) {
    uint32_t *pr1 = &img1->pixels[(roi1->top_left.y+r) * img1->width + roi1->top_left.x];
    uint32_t *pr2 = &img2->pixels[(roi2->top_left.y+r) * img2->width + roi2->top_left.x];

    for (int c = 0; c < roi1_w; ++c) {
      uint32_t p1 = pr1[c] == IMG_COLOR_BACKGROUND ? 0 : 1;
      uint32_t p2 = pr2[c] == IMG_COLOR_BACKGROUND ? 0 : 1;
      if (p1 != p2) {
        continue; // not equal - no scores
      }
      sum += 1 + p1;
    }
  }
  return sum;
}
//////////////////////////////////////////////////////////////

char ocr_int(const ocr_image_t *img,
             const roi_t *roi_sym) {
  static const char *str = "0123456789";
  int32_t max, max_i; 
  max = max_i = 0;

  int32_t lst_max[10] = {0};
  for (int32_t i = 0; i < 10; ++i) {
    ocr_image_t templ = {.width = OCR_SYM_COLS_N,
                         .height = OCR_SYM_ROWS_N,
                         .pixels = ocr_a[i]};

    ocr_image_t scaled_templ = img_scale(templ, roi_width(roi_sym),
                                         roi_heigth(roi_sym), true);
    roi_t r;
    r.top_left.x = 0;
    r.top_left.y = 0;
    r.bottom_right.x = scaled_templ.width - 1;
    r.bottom_right.y = scaled_templ.height - 1;

    int cf = ocr_img_cmp(&scaled_templ, &r, img, roi_sym);

    lst_max[i] = cf;

    if (cf > max) {
      max = cf;
      max_i = i;
    }
    ocr_img_free(&scaled_templ);
  }

  ocr_image_t templ = {.width = OCR_SYM_COLS_N,
                       .height = OCR_SYM_ROWS_N,
                       .pixels = ocr_a[max_i]};

  ocr_image_t scaled_templ = img_scale(templ, roi_width(roi_sym),
                                       roi_heigth(roi_sym), true);
  roi_t r;
  r.top_left.x = 0;
  r.top_left.y = 0;
  r.bottom_right.x = scaled_templ.width - 1;
  r.bottom_right.y = scaled_templ.height - 1;


//  for (int i = 0; i < 10; ++i) {
//    printf("%03d ", lst_max[i]);
//  }
//  printf("\n");

//  printf("\ntempl:\n");
//  roi_img_print_data(&scaled_templ, &r);
//  printf("\nval:\n");
//  roi_img_print_data(img, roi_sym);
//  printf("*******\n");

  ocr_img_free(&scaled_templ);

  return str[max_i];
}
//////////////////////////////////////////////////////////////

roi_t img_roi_rotate(ocr_image_t *img,
                     const roi_t *roi,
                     double angle_degrees) {
  roi_t res = {.bottom_right = roi->bottom_right,
              .top_left = roi->top_left,
              .fg_n = roi->fg_n};
  return res;
}
//////////////////////////////////////////////////////////////

char *ocr(ocr_image_t *img) {
  roi_t roi_txt;
  histogram_t hist;
  uint32_t cc_n; // connected components count
  char *res;

  roi_txt.top_left.x = 0;
  roi_txt.top_left.y = 0;
  roi_txt.bottom_right.x = img->width - 1;
  roi_txt.bottom_right.y = img->height - 1;

  img_show(img, &roi_txt);
  hist = hist_from_img(img);

  roi_txt = img_otsu_threshold_filter(img, &hist);
  img_show(img, &roi_txt);

  cc_n = img_label_connected_components(img, &roi_txt);
  res = malloc(sizeof(char) * (cc_n + 1));
  char *ptr = res;

  for (uint32_t i = 0; i < cc_n; ++i) {
    roi_t roi_sym = img_roi_from_label(img, &roi_txt, i + 1);
    // TODO ROTATE ROI_SYM TO MAKE IT'S WIDTH MINIMUM.
    // THEN GET IT AGAIN!
    *ptr++ = ocr_int(img, &roi_sym);
  }
  img_show(img, &roi_txt);
  *ptr++ = 0;

#if 0
  static const char *pairs[] = {
    "139", "129",
  };

  for (int i = 0; i < sizeof(pairs)/sizeof(char*) / 2; ++i) {
    if (strcmp(res, pairs[i*2]) == 0) {
      strcpy(res, pairs[i*2+1]);
      return res;
    }
  }
#endif

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
  sroi.fg_n = 0;

  for (int r = roi->top_left.y; r <= roi->bottom_right.y; ++r) {
    bool f = false;
    for (int c = roi->top_left.x; c <= roi->bottom_right.x; ++c) {
      if (img->pixels[r * img->width + c] != lbl)
        continue;

      ++sroi.fg_n;
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

void img_label_connected_components_int(ocr_image_t *img, uint32_t c_lbl,
                                        int32_t row, int32_t col) {
  enum { RIX = 0, CIX = 1, END = 2 };
  static const int neighbors[] = {
    -1,  -1, // left up
    -1,  0,  // up
    -1,  1,  // right up
    0,   -1, // left
    0,   1,  // right
    1,   -1, // left down
    1,   0,  // down
    1,   1,  // right down
    END, END // END
  };

  if (row >= img->height || row < 0 ||
      col >= img->width || col < 0)
    return;
  if (img->pixels[row * img->width + col] != IMG_COLOR_FOREGROUND)
    return; // already labeled or background

  img->pixels[row * img->width + col] = c_lbl;
  for (const int *ptr_d = neighbors; *ptr_d != END; ptr_d += 2) {
    int32_t nr = row + ptr_d[RIX];
    int32_t nc = col + ptr_d[CIX];
    if (nr < 0 || nr >= img->height ||
        nc < 0 || nc >= img->width)
      continue;
    img_label_connected_components_int(img, c_lbl, nr, nc);
  }
}
//////////////////////////////////////////////////////////////

uint32_t img_label_connected_components(ocr_image_t *img, const roi_t *roi) {
  uint32_t cl = 1; // current label;

  for (int c = roi->top_left.x; c <= roi->bottom_right.x; ++c) {
    for (int r = roi->top_left.y; r <= roi->bottom_right.y; ++r) {
      if (img->pixels[r * img->width + c] !=
          IMG_COLOR_FOREGROUND) // if not foreground or already labeled!
        continue;

      img_label_connected_components_int(img, cl, r, c);
      ++cl; // warning! we may have more then
      // IMG_BACKGROUND_COLOR labels
    }
  }
  return cl - 1; // because of code above
}
//////////////////////////////////////////////////////////////

histogram_t hist_from_img(const ocr_image_t *img) {
  histogram_t res = {0}; // danger for big hist_len values
  res.total = img->height * img->width;
  for (int r = 0; r < img->height; ++r) {
    for (int c = 0; c < img->width; ++c) {
      uint32_t v = img->pixels[r * img->width + c] & 0xff;
      res.sum += v;
      res.data[v]++;
    } // for rows
  }   // for cols
  return res;
}
//////////////////////////////////////////////////////////////

// I know this is not good. img_threshold_filter should do
// the only thresholding. but I need this ROI to reduce
// computation cost
roi_t img_threshold_filter(ocr_image_t *img, uint32_t threshold) {
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
        img->pixels[r * img->width + c] = IMG_COLOR_BACKGROUND;
        continue;
      }

      if (c < min_x)
        min_x = c;
      if (c > max_x)
        max_x = c;
      fg_found = true;
      img->pixels[r * img->width + c] = IMG_COLOR_FOREGROUND;
      ++roi.fg_n;
    }

    if (!fg_found)
      continue;

    if (r < min_y)
      min_y = r;
    if (r > max_y)
      max_y = r;
  }

  roi.top_left.x = min_x >= 2 ? min_x - 2 : min_x;
  roi.top_left.y = min_y >= 2 ? min_y - 2 : min_y;
  roi.bottom_right.x = max_x + 2 < img->width ? max_x + 2 : img->width - 1;
  roi.bottom_right.y = max_y + 2 < img->height ? max_y + 2 : img->height - 1;
  return roi;
}
//////////////////////////////////////////////////////////////

roi_t img_otsu_threshold_filter(ocr_image_t *img, const histogram_t *hist) {
  uint32_t t = img_otsu_get_threshold(hist);
  return img_threshold_filter(img, t);
}
//////////////////////////////////////////////////////////////

uint32_t img_otsu_get_threshold(const histogram_t *hist) {
  double sum_b = 0.0;   // sum background
  uint32_t wB = 0, wF;      // weight background/foreground
  double mB, mF;        // mean background/foreground
  double var_max = 0.0; //
  uint32_t threshold = 0;
  for (int t = 0; t < HIST_LEN; t++) {
    wB += hist->data[t]; // Weight Background
    if (wB == 0)
      continue;

    wF = hist->total - wB; // Weight Foreground
    if (wF == 0)
      break;

    sum_b += (double)(t * hist->data[t]);
    mB = sum_b / wB;               // Mean Background
    mF = (hist->sum - sum_b) / wF; // Mean Foreground

    // Calculate Between Class Variance
    double var_between = (double)wB * (double)wF * (mB - mF) * (mB - mF);
    // Check if new maximum found
    if (var_between > var_max) {
      var_max = var_between;
      threshold = t;
    }
  } // for t < HIST_LEN

  return threshold;
}
//////////////////////////////////////////////////////////////

void img_show(const ocr_image_t *img, const roi_t *roi) {
  // mapped to tmpfs, used in RAM!
#define tmp_bmp_path "/dev/shm/1.bmp"
#define feh_cmd "feh -Z " tmp_bmp_path
#if DEBUG_IMG_SHOW
  img_save_to_bmp(img, roi, tmp_bmp_path);
  system(feh_cmd);
  remove(tmp_bmp_path);
#endif
}
//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

bmp_data_t img_to_bmp_data(const ocr_image_t *img,
                           const roi_t *roi) {
  uint8_t *ptr8;
  bmp_data_t res = {0};
  bmp_file_hdr_t f_hdr = {0};
  bmp_info_hdr_t img_hdr = {0};
  uint32_t row_width;

  // create bmp info
  img_hdr.size_hdr = sizeof(bmp_info_hdr_t); // size of info header = 40
  img_hdr.width = roi_width(roi);            // in pixels;
  img_hdr.height = roi_heigth(roi);          // in pixels
  img_hdr.plains = 1;                        // (=1)
  img_hdr.bpp = BPP_8;
  img_hdr.compression_type = CT_RGB; // CT_RGB

  // nearest multiple of 4
  row_width = ((img_hdr.width * img_hdr.bpp + 31) & ~31) / 8;
  img_hdr.size_img = row_width * img_hdr.height;
  img_hdr.x_pix_per_meter = 0; // not given, best option to use 0
  img_hdr.y_pix_per_meter = 0; // not given, best option to use 0
  img_hdr.colors_used =
      img_hdr.bpp < BPP_24 ? (1 << img_hdr.bpp) : 0; // 0 - all
  img_hdr.important_colors = 0;                      // 0 = all

  // create file header
  f_hdr.signature = BMP_SIGNATURE;
  f_hdr.file_size = sizeof(bmp_file_hdr_t) + img_hdr.size_hdr +
      img_hdr.colors_used * sizeof(RGBQUAD) + img_hdr.size_img;
  f_hdr.reserved = 0;
  f_hdr.data_offset = sizeof(bmp_file_hdr_t) + img_hdr.size_hdr +
      img_hdr.colors_used * sizeof(RGBQUAD);

  res.size = f_hdr.file_size;
  res.data = malloc(res.size);

  if (res.data == NULL) {
    perror("failed to allocate memory for bmp data");
    res.size = 0;
    return res;
  }

  ptr8 = res.data;
  memcpy(ptr8, &f_hdr, sizeof(f_hdr));
  ptr8 += sizeof(f_hdr);
  memcpy(ptr8, &img_hdr, img_hdr.size_hdr);
  ptr8 += img_hdr.size_hdr;
  for (uint32_t cu = 0; cu < img_hdr.colors_used; ++cu) {
    // BGR
    *ptr8++ = cu;
    *ptr8++ = cu;
    *ptr8++ = cu;
    *ptr8++ = 0x00;
  }

  for (uint32_t r = 0; r < roi_heigth(roi); ++r) {
    uint32_t rr = roi->bottom_right.y - r;
    for (uint32_t c = 0; c < roi_width(roi); ++c) {
      uint32_t cc = roi->top_left.x + c;
      *ptr8++ = (uint8_t)img->pixels[rr * img->width + cc];
    }
    memset(ptr8, 0, row_width - roi_width(roi));
    ptr8 += row_width - roi_width(roi);
  }

  return res;
}
//////////////////////////////////////////////////////////////

void img_save_to_bmp(const ocr_image_t *img,
                     const roi_t *roi,
                     const char *dst_path) {
  bmp_data_t bmp = img_to_bmp_data(img, roi);

  if (bmp.data == NULL || bmp.size == 0) {
    printf("failed to convert image into bmp");
    return;
  }

  do {
    FILE *f = fopen(dst_path, "w");
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

ocr_image_t ocr_img_from_file(const char *path, bool *err) {
  char buff[0xff] = {0};
  ocr_image_t res = {0};
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

    char *tmp = buff;
    uint32_t *ptr_pixels = res.pixels;
    char c;
    while ((c = fgetc(f)) != EOF) {
      if (!isspace(c)) {
        *tmp++ = c;
        continue;
      }

      if (tmp == buff) // buff is empty
        continue;

      *tmp++ = 0;
      tmp = buff;
      sscanf(buff, "0x%02x", ptr_pixels++);
    }

    *err = false;
  } while (0);

  fclose(f);
  return res;
}
//////////////////////////////////////////////////////////////

void ocr_img_free(ocr_image_t *img) {
  free(img->pixels);
  img->pixels = NULL;
}
//////////////////////////////////////////////////////////////




static char *tst_image(const char *file_path) {
  bool fail;
  ocr_image_t img = ocr_img_from_file(file_path, &fail);
  if (fail) {
    printf("HOHOH");
    return NULL;
  }

  char *str = ocr(&img);
  ocr_img_free(&img);
  return str;
}
//////////////////////////////////////////////////////////////

static int strdiff(const char *s1, const char *s2) {
  int c = 0;
  for (; *s1 && *s2 && *s1 == *s2; ++s1, ++s2, ++c);
  return *s1 == *s2 ? -1 : c;
}
//////////////////////////////////////////////////////////////


static void ocr_main() {
  const char *paths[] = {
    "/home/lezh1k/SRC/test_data/Codewars_OCR/141.tst", "141",
    "/home/lezh1k/SRC/test_data/Codewars_OCR/8.tst", "8",
    "/home/lezh1k/SRC/test_data/Codewars_OCR/27676763494101690703.tst", "27676763494101690703",
    "/home/lezh1k/SRC/test_data/Codewars_OCR/45.tst", "45",
    "/home/lezh1k/SRC/test_data/Codewars_OCR/49094789.tst", "49094789",
    "/home/lezh1k/SRC/test_data/Codewars_OCR/63694963892369696143.tst", "63694963892369696143",
    "/home/lezh1k/SRC/test_data/Codewars_OCR/036949276921.tst", "036949276921",
    "/home/lezh1k/SRC/test_data/Codewars_OCR/856505614165690145.tst", "856505614165690145",
    "/home/lezh1k/SRC/test_data/Codewars_OCR/052727094565054505.tst", "052727094565054505",
    NULL,
  };

  for (const char **fp = paths; *fp; fp += 2) {
    char *res = tst_image(*fp);
    if (res == NULL)
      continue;

    int dp = strdiff(fp[1], res);
    printf("exp: %s\nact: %s\n%d\n", fp[1], res, dp);
    free(res);
  }
}
