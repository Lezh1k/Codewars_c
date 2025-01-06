#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include "trench_assault.h"

int main(int argc, const char *argv[]) {
  (void)argc;
  (void)argv;
  trench_assault_main(argc, argv);

  return 0;
}
