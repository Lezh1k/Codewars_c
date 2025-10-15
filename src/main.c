#include "inc_dec_sequences_count.h"
#include <stdalign.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;
  uint32_t N = 3;
  printf("%lu\n", total_inc_dec(N));
}
//////////////////////////////////////////////////////////////
