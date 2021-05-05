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
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

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
  float *mtx;
} convolutional_kernel_t;

#define HIST_LEN 0x100
typedef struct histogram {
  uint32_t data[HIST_LEN];
} histogram_t;

// AUX
static bmp_data_t img_to_bmp_data(const ocr_image_t *img);
static void img_save_to_bmp(const ocr_image_t *img,
                            const char *dst_path);
static void img_show(const ocr_image_t *img);

// MAIN
static histogram_t hist_from_img(const ocr_image_t *img);
static float hist_sigma(const histogram_t *hist);

static void img_apply_convolution(ocr_image_t *img,
                                  const convolutional_kernel_t *kernel);
static void img_blur(ocr_image_t *img, const histogram_t *hist);
static void img_otsu_binarization(ocr_image_t *img);

char*
ocr(ocr_image_t *img) {
  histogram_t hist = hist_from_img(img);
  img_show(img);
  img_blur(img, &hist);
  img_show(img);
  return "0";
}
//////////////////////////////////////////////////////////////

float
hist_sigma(const histogram_t *hist) {
  uint32_t N = 0;
  float mean, sigma;

  printf("*******************\n");
  for (int i = 0; i < HIST_LEN; ++i) {
    printf("%d ", hist->data[i]);
    if ((i+1) % 16 == 0)
      printf("\n");
  }
  printf("**********************\n");

  for (int i = 0; i < HIST_LEN; ++i) {
    N += hist->data[i];
  }
  mean = 0.0f;
  for (int i = 0; i < HIST_LEN; ++i) {
    mean += (hist->data[i] * i) / (float)N;
  }

  sigma = 0.0f;
  for (int i = 0; i < HIST_LEN; ++i) {
    float t = i - mean;
    sigma += t * t;
  }

  sigma = sqrtf(1.f / N * sigma);
  return sigma;
}
//////////////////////////////////////////////////////////////

histogram_t
hist_from_img(const ocr_image_t *img) {
  histogram_t res = {0}; //danger for big hist_len values
  for (int r = 0; r < img->height; ++r) {
    for (int c = 0; c < img->width; ++c) {
      uint32_t v = img->pixels[r * img->width + c];
      v = v > HIST_LEN ? HIST_LEN : v; // not necessary
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
  float sum;

  for (r = 0; r < img->height; ++r) {
    for (c = 0; c < img->width; ++c) {
      sum = 0.0f;

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
  float sigma = hist_sigma(hist);
  float sigma_2 = sigma*sigma;
  float sum = 0.0;
  convolutional_kernel_t kernel;
  kernel.height = kernel.width = BLUR_N;
  kernel.mtx = malloc(sizeof(float) * BLUR_N * BLUR_N);

  for (size_t r = 0; r < kernel.height; ++r) {
    for (size_t c = 0; c < kernel.width; ++c) {
      int y = (int)r - kernel.height / 2;
      int x = (int)c - kernel.width / 2;

      float pow = - (x*x + y*y) / (2.f * sigma_2);
      float G = (1.f / (M_2_PI * sigma_2)) * expf(pow);

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

void
img_otsu_binarization(ocr_image_t *img) {

}
//////////////////////////////////////////////////////////////

void img_show(const ocr_image_t *img)  {
  // mapped to tmpfs, used in RAM!
#define tmp_bmp_path "/dev/shm/1.bmp"
#define feh_cmd "feh "tmp_bmp_path

  img_save_to_bmp(img, tmp_bmp_path);
  system(feh_cmd);
  remove(tmp_bmp_path);
}
//////////////////////////////////////////////////////////////

bmp_data_t
img_to_bmp_data(const ocr_image_t *img) {
  uint8_t *ptr8;
  bmp_data_t res = {0};
  bmp_file_hdr_t f_hdr = {0};
  bmp_info_hdr_t img_hdr = {0};
  uint32_t row_width;

  // create bmp info
  img_hdr.size_hdr = sizeof(bmp_info_hdr_t); //size of info header = 40
  img_hdr.width = img->width; // in pixels;
  img_hdr.height = img->height; // in pixels
  img_hdr.plains = 1; // (=1) ??
  img_hdr.bpp = BPP_8;
  img_hdr.compression_type = CT_RGB; // CT_RGB
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
    *ptr8++ = cu;
    *ptr8++ = cu;
    *ptr8++ = cu;
    *ptr8++ = cu;
  }

  for (int r = img->height - 1; r >= 0; --r) {
    for (int c = 0; c < img->width; ++c)
      *ptr8++ = (uint8_t) img->pixels[r * img->width + c];
    memset(ptr8, 0, row_width - img->width);
    ptr8 += row_width - img->width;
  }
  return res;
}
//////////////////////////////////////////////////////////////

void
img_save_to_bmp(const ocr_image_t *img,
                const char *dst_path) {
  bmp_data_t bmp = img_to_bmp_data(img);
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

