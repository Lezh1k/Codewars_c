#ifndef OCR_H
#define OCR_H

#include <stdint.h>
#include <stdbool.h>

typedef struct OcrImage { int width, height; uint32_t *pixels; } ocr_image_t;
extern const unsigned IMAGE_MAX;

ocr_image_t ocr_img_from_file(const char *path, bool *err);
void ocr_img_free(ocr_image_t *img);

// Extract a text string from an image of text.
// Return value is a pointer to a newly-allocated string, which should be freed after use.
char *ocr(ocr_image_t *image);

#endif // OCR_H
