#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#define FILL_CHAR_EMPTY -1
char *str_u8_n(uint8_t val, char *buff, uint8_t n, char fill_char)
{
  assert(n);
  buff += --n;
  *buff-- = 0;

  if (val == 0) {
    *buff-- = '0';
    --n;
  }

  while (val && n--) {
    *buff-- = '0' + (val % 10);
    val /= 10;
  }

  if (fill_char == FILL_CHAR_EMPTY)
    return ++buff;

  while (n--) {
    *buff-- = fill_char;
  }

  return ++buff;
}
//////////////////////////////////////////////////////////////

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  char buff[3] = {0};
  printf("%p\n, %c\n", (void*)buff, *buff);
  char *s = str_u8_n(12, buff, 3, '0');
  printf("%s\n", s);
  printf("%p\n, %c\n", (void*)buff, *buff);

  return 0;
}
//////////////////////////////////////////////////////////////
