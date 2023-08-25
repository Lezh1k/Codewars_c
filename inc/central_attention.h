#ifndef CENTRAL_ATTENTION_H
#define CENTRAL_ATTENTION_H

#include <stdint.h>

typedef struct { unsigned *pixels; unsigned width, height; } Image;
typedef struct { unsigned *values; unsigned size; } unsigned_array;

unsigned_array central_pixels(Image image, unsigned colour);
void print_image(const Image *img);

#endif // CENTRAL_ATTENTION_H
