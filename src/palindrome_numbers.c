#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "palindrome_numbers.h"

static char *ull_to_str(ull_t n, char buff[19]);

ull_t find_reverse_number(ull_t n) {
  assert(n > 0);
  if (n <= 10) {
    return n - 1;
  }
  /*
   * If the palindrome has an even number of digits, prepend a 1 to the front
   * half of the palindrome's digits. If the number of digits is odd, prepend
   * the value of front digit + 1 to the digits from position 2 ... central
   * digit. Examples: 9876006789 = a(198760), 515 = a(61), 8206028 = a(9206),
   * 9230329 = a(10230).
   * even when val % 2 == 0
   * odd when val % 2 == 1
   */
  char buff[19] = {0};
  char res_str[19] = {0};

  char *str = ull_to_str(n, buff);
  ul_t slen = strlen(str);

  if (*str != '1' || (*str == '1' && slen >= 2 && str[1] == '0')) {
    if (--(*str) == '0') {
      *++str = '9';
      --slen;
    }
    char *pr = res_str;
    for (char *ts = str; *ts; ++ts)
      *pr++ = *ts;
    // because last symbol of str is the middle symbol of result
    for (char *ts = str + slen - 2; ts >= str; --ts)
      *pr++ = *ts;
    *pr++ = 0; // not necessary
  } else {
    ++str;
    --slen;
    char *pr = res_str;
    for (char *ts = str; *ts; ++ts)
      *pr++ = *ts;
    for (char *ts = str + slen - 1; ts >= str; --ts)
      *pr++ = *ts;
    *pr++ = 0; // not necessary
  }

  return strtoull(res_str, NULL, 10);
}
//////////////////////////////////////////////////////////////

char *ull_to_str(ull_t n, char buff[19]) {
  char *pb = &buff[18];
  while (n) {
    *--pb = n % 10 + '0';
    n /= 10;
  }
  return pb;
}
//////////////////////////////////////////////////////////////
