#include "ocr.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

int main_(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  bool fail;
  ocr_image_t img =
      ocr_img_from_file("/home/lezh1k/SRC/test_data/Codewars_OCR/0.tst", &fail);
  if (fail) {
    printf("HOHOH");
    return -1;
  }

  struct timeval stop, start;
  gettimeofday(&start, NULL);

  char *str = ocr(&img);
  printf("%s\n", str);
  free(str);
  gettimeofday(&stop, NULL);

  printf("took %lu us\n",
         (stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec);
  printf("SOLVED\n");
  ocr_img_free(&img);
  return 0;
}
