#include "ocr.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

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

int
main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  const char *paths[] = {
//    "/home/lezh1k/SRC/test_data/Codewars_OCR/141.tst", "141",
//    "/home/lezh1k/SRC/test_data/Codewars_OCR/8.tst", "8",
//    "/home/lezh1k/SRC/test_data/Codewars_OCR/27676763494101690703.tst", "27676763494101690703",
//    "/home/lezh1k/SRC/test_data/Codewars_OCR/45.tst", "45",
//    "/home/lezh1k/SRC/test_data/Codewars_OCR/49094789.tst", "49094789",
//    "/home/lezh1k/SRC/test_data/Codewars_OCR/63694963892369696143.tst", "63694963892369696143",
//    "/home/lezh1k/SRC/test_data/Codewars_OCR/036949276921.tst", "036949276921",
//    "/home/lezh1k/SRC/test_data/Codewars_OCR/856505614165690145.tst", "856505614165690145",
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

  return 0;
}
