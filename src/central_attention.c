#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include "central_attention.h"

#define MIN(x, y) ((x < y ? x : y))
#define MAX(x, y) ((x < y ? y : x))

static void pushToRes(unsigned_array *res, int *min, int item, unsigned itIx);

static void st1ProcessEdgeRow(unsigned *row, unsigned imgW,
                              unsigned colour, unsigned_array *res);

void st1ProcessEdgeRow(unsigned *row, unsigned imgW,
                       unsigned colour, unsigned_array *res) {
  for (unsigned x = 0; x < imgW; ++x) {
    if (row[x] != colour) continue;
    row[x] = (unsigned)-1;
    ++res->size;
  }
}
///////////////////////////////////////////////////////

void pushToRes(unsigned_array *res,
               int *lpMinDepth,
               int item,
               unsigned itemIndex) {
  if (item > *lpMinDepth) return;
  if (item < *lpMinDepth) {
    res->size = 0;
    *lpMinDepth = item;
  }
  res->values[res->size++] = itemIndex;
}
///////////////////////////////////////////////////////

unsigned_array central_pixels(Image image,
                              unsigned colour) {
#define VISITED(x) (((signed)x) < 0)

  unsigned_array res = {.size = 0, .values = NULL};
  unsigned *curRow, *prevRow;
  curRow = prevRow = image.pixels;
  unsigned x, y;
  x = y = 0;

  //!first pass
  st1ProcessEdgeRow(curRow, image.width, colour, &res);
  curRow += image.width;
  for (y = 1; y < image.height-1; ++y, curRow += image.width, prevRow += image.width) {
    x = 0;
    if (curRow[x] == colour) {
      curRow[x] = (unsigned)-1;
      ++res.size;
    }

    for (x = 1; x < image.width-1; ++x) {
      if (curRow[x] != colour)
        continue;

      if ((!VISITED(prevRow[x]) && prevRow[x] != colour) ||
          (!VISITED(curRow[x-1]) && curRow[x-1] != colour)) {
        curRow[x] = (unsigned)-1;
        ++res.size;
        continue;
      }

      signed max = MAX((signed)prevRow[x], (signed)curRow[x-1]); //unsigned min!
      curRow[x] = (unsigned)--max;
      ++res.size;
    }

    if (curRow[x] == colour) {
      curRow[x] = (unsigned)-1;
      ++res.size;
    }
  }
  st1ProcessEdgeRow(curRow, image.width, colour, &res);
  ///////////////////////////////////////////////////////

  res.values = malloc(res.size*sizeof(unsigned));
  res.size = 0;

  int minDepth = -1;
  unsigned li = image.width*image.height - 1; //last index;

  //!second pass reverse order
  prevRow = curRow;
  for (int x = (int)(image.width-1); x >= 0; --x, --li) {
    if (!VISITED(curRow[x])) continue;
    pushToRes(&res, &minDepth, (signed)curRow[x], li);
  }

  curRow -= image.width;
  for (y = 1; y < image.height-1; ++y, curRow -= image.width, prevRow -= image.width) {
    x = image.width-1;
    if (VISITED(curRow[x])) {
      curRow[x] = (unsigned)-1;
      pushToRes(&res, &minDepth, (signed)curRow[x], li);
    }
    --li;

    for (x = image.width-2; x >= 1; --x, --li) {
      if (!VISITED(curRow[x]))
        continue;

      if (!VISITED(prevRow[x]) ||
          !VISITED(curRow[x+1])) {
        curRow[x] = (unsigned)-1;
        pushToRes(&res, &minDepth, (signed)curRow[x], li);
        continue;
      }

      signed max = MAX((signed)prevRow[x], (signed)curRow[x+1]); //unsigned min!
      max = MAX((signed)(max-1), (signed)curRow[x]);
      curRow[x] = (unsigned)max;
      pushToRes(&res, &minDepth, (signed)curRow[x], li);
    } //for x

    //last row
    if (VISITED(curRow[x])) {
      curRow[x] = (unsigned)-1;
      pushToRes(&res, &minDepth, (signed)curRow[x], li);
    }
    --li;
  } //for y

  for (int x = (int)(image.width-1); x >= 0; --x, --li) {
    if (!VISITED(curRow[x])) continue;
    pushToRes(&res, &minDepth, (signed)curRow[x], li);
  }
  ///////////////////////////////////////////////////////

  if (res.size) {
    unsigned mid = res.size / 2;
    for (unsigned i = 0; i < mid; ++i) {
      unsigned tmp = res.values[i];
      res.values[i] = res.values[res.size-i-1];
      res.values[res.size-i-1] = tmp;
    }
  }

  unsigned *tmp = image.pixels;
  for (y = 0; y < image.height; ++y) {
    for (x = 0; x < image.width; ++x, ++tmp) {
      if (!VISITED(*tmp)) continue;
      *tmp = colour;
    }
  }

  return res;
}
///////////////////////////////////////////////////////

void print_image(const Image *img) {
  unsigned *tmp = img->pixels;
  for (unsigned y = 0; y < img->height; ++y) {
    for (unsigned x = 0; x < img->width; ++x) {
      printf("%d,\t", (signed)*tmp++);
    }
    printf("\n");
  }
}
