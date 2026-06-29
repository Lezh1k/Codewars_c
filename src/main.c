#include <assert.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include "asm_interpret.h"

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;
  // printf("qr generator\n");
  asm_interpret_main(argc, argv);
  return 0;
}
//////////////////////////////////////////////////////////////
