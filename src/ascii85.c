#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include <math.h>
#include "ascii85.h"

static inline int nearestMultipleOf4(int v) {
  return (v+3) & ~3;
}

//very bad idea. we assume that all ascii85 strings end with "~>"
static inline int ascii85len(const char *src) {
  int res = 0;
  while (!(src[res]=='~' && src[res+1]=='>'))
    ++res;
  return res-2;
}
//////////////////////////////////////////////////////////////////////////

char *toAscii85 (block b) {
  int m, i;
  uint8_t *dst, *data;
  m = (nearestMultipleOf4(b.n)/4)*5+5;
  data = malloc((size_t)m); //for <~~>\0
  dst = data;
  *dst++ = '<';
  *dst++ = '~';

  while (b.n > 0) {\
    uint32_t v = 0;
    dst[0]=dst[1]=dst[2]=dst[3]=dst[4]=0;

    switch (b.n) {
      default:
        v |= (uint32_t)((uint8_t)b.data[3]);        
        /* fall through */
      case 3:
        v |= (uint32_t)((uint8_t)b.data[2]) << 8;
        /* fall through */
      case 2:
        v |= (uint32_t)((uint8_t)b.data[1]) << 16;
        /* fall through */
      case 1:
        v |= (uint32_t)((uint8_t)b.data[0]) << 24;
    }

    if (v == 0 && b.n >= 4) {
      *dst++ = 'z';
      b.data += 4;
      b.n -= 4;
      continue;
    }

    for (i = 4; i >= 0; --i) {
      dst[i] = '!' + (uint8_t)(v%85);
      v /= 85;
    }

    m = 5;
    if (b.n < 4) {
      m -= 4 - b.n;
      b.n = 0;
    } else {
      b.n -= 4;
      b.data += 4;
    }
    dst += m;
  }

  *dst++ = '~';
  *dst++ = '>';
  *dst++ = '\0';

  for (i = 0; i < (int)(dst-data); ++i) {
    printf("%d ", data[i]);
  }
  printf("\n");
  return (char*)data;
}
//////////////////////////////////////////////////////////////////////////

block fromAscii85 (const char *in) {
  block res;
  uint32_t v = 0;
  int nb, ndst, i;
  uint8_t *data;
  const uint8_t *src = (const uint8_t*)in;
  res.n = ascii85len(in);
  data = malloc((size_t)res.n+1); //for \0
  res.data = (char*)data;
  nb = ndst = 0;
  src += 2; //<~

  for (; *src != '~'; ++src) {
    if (res.n - ndst < 4) {
      res.n = ndst;
      return res;
    }

    if (*src <= ' ')
      continue;

    if (*src == 'z' && nb == 0) {
      nb = 5;
      v = 0;
    }

    if (*src >= '!' && *src <= 'u') {
      v = v*85 + (uint32_t)(*src-'!');
      ++nb;
    }

    if (nb==5) {
      data[ndst] = (uint8_t) (v >> 24);
      data[ndst+1] = (uint8_t) (v >> 16);
      data[ndst+2] = (uint8_t) (v >> 8);
      data[ndst+3] = (uint8_t) (v);
      ndst += 4;
      nb = 0;
      v = 0;
    }
  }

  for (i = nb; i < 5; ++i) {
    v = v*85+84;
  }

  for (i = 0; i < nb-1; ++i) {
    data[ndst++] = v >> 24;
    v <<= 8;
  }

  res.n = ndst;
  return res;
}
//////////////////////////////////////////////////////////////////////////
