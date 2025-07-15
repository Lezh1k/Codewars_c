#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// #include <xmmintrin.h>
#include <immintrin.h>
#include <emmintrin.h>
#include <xmmintrin.h>

static bool calloc_a16(void **ptr_allocated, void **ptr_aligned, size_t n) {
  *ptr_allocated = calloc(n + 16, 1);
  if (!*ptr_allocated) {
    return false;
  }
  uint64_t pa = (uint64_t)*ptr_allocated;
  pa = (pa + 15) & ~15;
  *ptr_aligned = (void *)pa;
  return true;
}
//////////////////////////////////////////////////////////////

int main(int argc, const char *argv[]) {
  (void)argc;
  (void)argv;
  const char *input =
      "HHello, WORLd!!!!Hello, WORLd!!!!Hello, WORLd!!!!Hello, WORLd!!!!Hello, "
      "WORLd!!!!Hello, WORLd!!!!ello, WORLd!!!!";
  size_t input_len = strlen(input);
  void *output_allocated, *output_aligned;
  if (!calloc_a16(&output_allocated, &output_aligned, input_len + 16)) {
    perror("failed to allocate aligned memory");
    return 1;
  }

  printf("%p ::: %p\n", output_allocated, output_aligned);
  printf("%lu ::: %lu\n", (uint64_t)output_allocated % 16,
         (uint64_t)output_aligned % 16);

  memcpy(output_aligned, input, input_len);
  for (char *pout = output_aligned; *pout; pout += 16) {
    __m128 reg = _mm_load_ps((const float *)pout);
    printf("%s\n", pout);
  }

  // for (char *pc = output_aligned; *pc; ++pc) {
  //   unsigned char bh = *pc;
  //   bh |= 0x20;
  //   bh -= 'a';
  //   if (bh > ('z' - 'a')) {
  //     bh += 'a';
  //     printf("%c", bh);
  //     continue;
  //   }
  //
  //   bh += 'a' + 13;
  //   if (bh > 'z') {
  //     bh -= 26;
  //   }
  //
  //   if (*pc < 'a') {
  //     bh -= 32;
  //   }
  //
  //   printf("%c", bh);
  // }

  free(output_allocated);
  return 0;
}
//////////////////////////////////////////////////////////////
