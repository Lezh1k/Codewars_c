#include "ocr.h"

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <fcntl.h>

#include <unistd.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <X11/Xlib.h>

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

typedef struct screen {
  char    *buffer;
  size_t  size, bytes_per_pixel, bytes_per_line,
  width, height;
  uint_fast16_t    red, green, blue;
} screen_t;

static void img_save_to_bmp(const ocr_image_t *img, const char *dst_path) __attribute_used__;
static void img_display_fb(const ocr_image_t *img, const char *fbdev) __attribute_used__;

static void img_display_X(const ocr_image_t *img);

char*
ocr(ocr_image_t *img) {
  img_save_to_bmp(img, "/home/lezh1k/1.bmp");
  return "0";
}
//////////////////////////////////////////////////////////////

void
img_display_X(const ocr_image_t *img) {
  (void) img;
}
//////////////////////////////////////////////////////////////

void
img_save_to_bmp(const ocr_image_t *img,
                const char *dst_path) {
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

  FILE *f = fopen(dst_path, "w");
  if (f == NULL) {
    perror("failed to open file for writing");
    return;
  }

  do {
    fwrite(&f_hdr, 1, sizeof(f_hdr), f);
    fwrite(&img_hdr, 1, img_hdr.size_hdr, f);

    // write color table gray scale
    if (img_hdr.bpp <= BPP_8) {
      for (uint32_t cu = 0; cu < img_hdr.colors_used; ++cu) {
        RGBQUAD q = {.rgbBlue = cu, .rgbGreen = cu,
          .rgbRed = cu,.rgbReserved = 0};
        fwrite(&q, 1, sizeof(RGBQUAD), f);
      }
    }

    for (int r = img->height - 1; r >= 0; --r) {
      fwrite((void*)&img->pixels[r * img->width], 1, row_width, f);
    }
  } while (0);

  fclose(f);
}
//////////////////////////////////////////////////////////////

ocr_image_t
ocr_img_from_file(const char *path,
                  bool *err) {
  char buff[0xff] = {0};
  char *tmp, c;
  ocr_image_t res = {0};
  uint8_t *ptr_pixels;
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

    res.pixels = malloc(sizeof(uint8_t) * res.width * res.height);
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
      sscanf(buff, "0x%2s", ptr_pixels++);
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

void
img_display_fb(const ocr_image_t *img,
               const char *fbdev) {
  int fbfd = open (fbdev, O_RDWR);
  if (fbfd < 0) {
    perror("failed to open fbdev");
    return;
  }

  do {
    struct fb_var_screeninfo vinf;
    struct fb_fix_screeninfo finf;

    if (ioctl (fbfd, FBIOGET_FSCREENINFO, &finf) == -1) {
      perror("failed to open fixed screen info");
      break;
    }

    if (ioctl (fbfd, FBIOGET_VSCREENINFO, &vinf) == -1) {
      perror("failed to open variable screen info");
      break;
    }

    screen_t s = {
      .size            = finf.line_length * vinf.yres,
      .bytes_per_pixel = vinf.bits_per_pixel / 8,
      .bytes_per_line  = finf.line_length,
      .red             = vinf.red.offset/8,
      .green           = vinf.green.offset/8,
      .blue            = vinf.blue.offset/8,
      .width           = vinf.xres,
      .height          = vinf.yres
    };
    s.buffer = mmap(0, s.size, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);

    if (s.buffer == MAP_FAILED) {
      perror("failed to map frame buffer");
      break;
    }

    uint_fast32_t start_x = vinf.xres / 2 - img->width / 2;
    uint_fast32_t start_y = vinf.yres / 2 - img->height / 2;
    for (int r = 0; r < img->height; r++) {
      for (int c = 0; c < img->width; c++) {
        uint_fast32_t pix_offset = (c+start_x) * s.bytes_per_pixel + (r+start_y) * s.bytes_per_line;
        s.buffer[pix_offset + s.red] = img->pixels[r*img->width + c];
        s.buffer[pix_offset + s.green] = img->pixels[r*img->width + c];
        s.buffer[pix_offset + s.blue] = img->pixels[r*img->width + c];
      }
    }

    vinf.activate = FB_ACTIVATE_NOW | FB_ACTIVATE_FORCE;
    if ((ioctl( fbfd, FBIOPUT_VSCREENINFO, &vinf)) == -1) {
      perror("failed to activate framebuffer");
    }

    printf("press any key to continue...\n");
    getchar(); // wait for any key
    munmap(s.buffer, s.size);
  } while(0);

  close (fbfd);
}
//////////////////////////////////////////////////////////////
