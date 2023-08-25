#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parse_int.h"

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;
  const char *tst_strings[] = {
      "six hundred sixty-six thousand six hundred sixty-six",
      "four",
      "seven",
      "twenty-one thousand five hundred and nineteen",
      "thirty-two hundred thousand three",
      "two hundred thousand",
      NULL};

  for (const char **str = tst_strings; *str; ++str) {
    long num = parse_int(*str);
    printf("%s => %ld\n", *str, num);
    printf("***************\n");
  }
  // parse_int("twenty one");
  return 0;
}
//////////////////////////////////////////////////////////////
