#include "ocr.h"

#include <MagickWand/MagickWand.h>

char*
ocr(ocr_image_t *image) {

  MagickWandGenesis();
  MagickWand *wand;
  wand = NewMagickWand();
  MagickReadImageBlob(wand, image->pixels, image->height*image->width);
  MagickQuantizeImage(wand, 255, GRAYColorspace, 0, NoDitherMethod, MagickTrue);
  MagickDisplayImage(wand, ":0");
  wand = DestroyMagickWand(wand);
  MagickWandTerminus();
  return "0";
}
