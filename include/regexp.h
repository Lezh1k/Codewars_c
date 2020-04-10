#ifndef REGEXP_H
#define REGEXP_H

#include <stdint.h>

#define SS_POSSIBLE_BYTE_VALUES_COUNT 0x100
#define SS_OFFSET -16-SS_POSSIBLE_BYTE_VALUES_COUNT
enum special_states_t { //got all values from ktulhu
  SS_CONCAT = SS_OFFSET - 0,
  SS_DOT    = SS_OFFSET - 1,
  SS_SPLIT  = SS_OFFSET - 2,
  SS_MATCH  = SS_OFFSET - 3,
  SS_START  = SS_OFFSET - 4
};

#define ERR_WRONG_REGEXP -1
#define ERR_WRONG_PAREN -2

typedef struct nfa_state nfa_state_t;
struct nfa_state {
  int32_t signal;
  nfa_state_t *out;
  nfa_state_t *out1;
  int32_t index;
};
extern nfa_state_t match_state;

typedef union ptr_list ptr_list_t;
union ptr_list {
  ptr_list_t *next;
  nfa_state_t *state;
};
/* Create ptr_list from aux_state*/
ptr_list_t *pl_list(nfa_state_t **outp);

/* Patch the list of states at out to point to start. */
void pl_patch(ptr_list_t *lst, nfa_state_t *s);

/* Join the two lists l1 and l2, returning the combination. */
ptr_list_t *pl_append(ptr_list_t *l, ptr_list_t *r);


nfa_state_t* nfa_state(int32_t signal,
                       nfa_state_t *out,
                       nfa_state_t *out1,
                       int32_t index);

int nfa_state_cmp(const nfa_state_t *l,
                  const nfa_state_t *r);

typedef struct frag frag_t;
struct frag {
  nfa_state_t *start;
  ptr_list_t *out;
};
frag_t frag(nfa_state_t *start, ptr_list_t *out);

///////////////////////////////////////////////////////
///////////////////////////////////////////////////////

int
regex2postfix(const char *regexp,
              int16_t **result,
              int *len);

nfa_state_t*
regexp_postfix2nfa(const int16_t *lst,
                   int len);

///////////////////////////////////////////////////////


#endif // REGEXP_H
