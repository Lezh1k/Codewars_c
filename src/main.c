#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc, const char *argv[]) {
  (void)argc;
  (void)argv;
  const char *input = "test";
  char *output = strdup(input);

  for (char *pc = output; *pc; ++pc) {
    unsigned char bh = *pc;
    bh |= 0x20;
    bh -= 'a';
    if (bh > ('z' - 'a')) {
      bh += 'a';
      printf("%c", bh);
      continue;
    }

    bh += 'a' + 13;
    if (bh > 'z') {
      bh -= 26;
    }

    if (*pc < 'a') {
      bh -= 32;
    }

    printf("%c", bh);
  }
  return 0;
}
//////////////////////////////////////////////////////////////
