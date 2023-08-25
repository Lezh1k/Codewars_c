#ifndef REGEXP_H
#define REGEXP_H

#include <stdint.h>
#include <stddef.h>

#define SS_POSSIBLE_BYTE_VALUES_COUNT 0x100
#define SS_OFFSET -16-SS_POSSIBLE_BYTE_VALUES_COUNT
enum special_states_t { //got all values from ktulhu
  SS_CONCAT = SS_OFFSET - 0,
  SS_DOT    = SS_OFFSET - 1,
  SS_SPLIT  = SS_OFFSET - 2,
  SS_MATCH  = SS_OFFSET - 3,
  SS_START  = SS_OFFSET - 4,
  SS_PAREN  = SS_OFFSET - 5,
  SS_IGNORE = SS_OFFSET - 6,
};

#define ERR_WRONG_REGEXP -1
#define ERR_WRONG_PAREN -2

int
regex2postfix(const char *regexp,
              int16_t **result,
              int *len, char symbol_to_ignore);

///////////////////////////////////////////////////////

typedef struct RegExp RegExp;
RegExp* parseRegExp (const char *input);

#endif // REGEXP_H
