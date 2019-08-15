#include <stdint.h>
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "central_attention.h"


int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  setbuf(stdout, NULL);
//  unsigned image_data[] = {1,1,4,4,4,4,2,2,2,2,
//                           1,1,1,1,2,2,2,2,2,2,
//                           1,1,1,1,2,2,2,2,2,2,
//                           1,1,1,1,1,3,2,2,2,2,
//                           1,1,1,1,1,3,3,3,2,2,
//                           1,1,1,1,1,1,3,3,3,3};
//  Image image = { image_data, 10, 6 };
  unsigned image_data[] = {1,1,1,
                           1,1,1,
                           2,2,2};
  Image image = { image_data, 3, 3 };
  unsigned_array arr = central_pixels(image, 1);

  if (arr.values != NULL) {
    for (unsigned i = 0; i < arr.size; ++i)
      printf("%d ", arr.values[i]);
    printf("\n");
    free(arr.values);
  }
}
