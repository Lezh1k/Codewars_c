#ifndef OCR_H
#define OCR_H

#include <stdint.h>

typedef struct OcrImage { int width, height; unsigned *pixels; } ocr_image_t;
extern const unsigned IMAGE_MAX;

// Extract a text string from an image of text.
// Return value is a pointer to a newly-allocated string, which should be freed after use.
char *ocr(ocr_image_t *image);

#endif // OCR_H
