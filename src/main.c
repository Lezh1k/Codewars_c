#include <stdalign.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include "prime_stream_simple.h"

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;
  struct prime_stream *ps = prime_stream_new();
  for (uint32_t i = 0; i < 1000000; ++i) {
    printf("%u\t", prime_stream_next(ps));
    if ((i + 1) % 10 == 0)
      printf("\n");
  }
}
//////////////////////////////////////////////////////////////
