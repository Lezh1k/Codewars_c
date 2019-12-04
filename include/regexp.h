#ifndef REGEXP_H
#define REGEXP_H

#define POSSIBLE_BYTE_VALUES_COUNT 0x100
enum { SS_CONCAT = -16-POSSIBLE_BYTE_VALUES_COUNT };

#define ERR_WRONG_REGEXP -1
#define ERR_WRONG_PAREN -2

int
regex2postfix(const char *regexp,
              int **result,
              int *len);

#endif // REGEXP_H
