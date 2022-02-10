#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "regexp.h"

typedef struct paren {
  int nalt;
  int natom;
} paren_t;
static paren_t paren(void);

paren_t paren() {
  paren_t r = {.nalt = 0, .natom = 0};
  return r;
}
///////////////////////////////////////////////////////

int
regex2postfix(const char *regexp,
              int16_t **result,
              int *len,
              char symbol_to_ignore) {
  static paren_t st_paren[0xff] = {0}; //warning! unsafe
  paren_t *paren_sp = st_paren;
#define paren_push(x) (*paren_sp++ = x)
#define paren_pop() (*--paren_sp)
#define paren_empty() (paren_sp == st_paren)

  //n alternatives , n atoms
  //atom is any symbol except ()|*?+
  int nalt, natom;
  int16_t *res = malloc(sizeof(int16_t) * strlen(regexp)*2);
  *result = res;
  paren_t p = paren();
  //in curent parentheses
  nalt = natom = 0;

  for(;*regexp;regexp++){

    if (*regexp == symbol_to_ignore) {
      if(natom > 1){
        --natom;
        *res++ = SS_CONCAT;
      }
      *res++ = SS_IGNORE;
      ++natom;
      continue;
    }

    switch(*regexp) {
      case '(':
        if(natom > 1){
          --natom;
          *res++ = SS_CONCAT;
        }
        p.nalt = nalt;
        p.natom = natom;
        paren_push(p);
        nalt = 0;
        natom = 0;
        break;

      case '|':
        if(natom == 0)
          return ERR_WRONG_REGEXP;
        while(--natom > 0)
          *res++ = SS_CONCAT;
        nalt++;
        break;

      case ')':
        if(paren_empty())
          return ERR_WRONG_PAREN; //wrong parentheses!
        if(natom == 0)
          return ERR_WRONG_REGEXP; //wrong regexp. () - empty parentheses

        while(--natom > 0)
          *res++ = SS_CONCAT;

        for(;nalt > 0; --nalt)
          *res++ = '|';

        p = paren_pop();
        nalt = p.nalt;
        natom = p.natom;
        natom++;
        break;

      case '*':
//      case '+':
//      case '?':
        if(natom == 0)
          return ERR_WRONG_REGEXP; //wrong regexp
        *res++ = *regexp;
        if (*(regexp-1) == '*' || *(regexp-1) == '+')
        break;

      default: //any character, except escape, ()|+?*
        if(natom > 1){
          --natom;
          *res++ = SS_CONCAT;
        }
        *res++ = *regexp;
        ++natom;
        break;
    }
  }

  if(!paren_empty())
    return ERR_WRONG_PAREN;

  while(--natom > 0)
    *res++ = SS_CONCAT;

  for(; nalt > 0; nalt--)
    *res++ = '|';

  *len = (int)(res-(*result));
  return 0;
}
///////////////////////////////////////////////////////

typedef struct nfa_state {
  int32_t signal;
  struct nfa_state *out1;
  struct nfa_state *out2;
} nfa_state_t;

static nfa_state_t *
nfa_state(int32_t signal_,
          nfa_state_t *out1,
          nfa_state_t *out2){
  nfa_state_t *res = malloc(sizeof(nfa_state_t));
  res->signal = signal_;
  res->out1 = out1;
  res->out2 = out2;
  return res;
}

typedef union ptrlst_nfa_states {
  union ptrlst_nfa_states *next;
  nfa_state_t *state;
} ptrlst_nfa_states_t;

typedef struct frag  {
  nfa_state_t *start;
  ptrlst_nfa_states_t *out;
} frag_t;

static frag_t
frag(nfa_state_t *start,
     ptrlst_nfa_states_t *out) {
  frag_t res = {.start = start, .out = out};
  return res;
}

/* Create ptr_list from aux_state*/
static ptrlst_nfa_states_t*
ptrlst_constr(nfa_state_t **outp) {
  ptrlst_nfa_states_t *l;
  l = (ptrlst_nfa_states_t *)outp;
  l->next = NULL;
  return l;
}
////////////////////////////////////////////////////////////////////////////

/* Patch the list of states at out to point to start. */
static void
ptrlst_patch(ptrlst_nfa_states_t *lst,
             nfa_state_t *s) {
  ptrlst_nfa_states_t *next;
  for(;lst;lst=next){
    next=lst->next;
    lst->state=s;
  }
}
////////////////////////////////////////////////////////////////////////////

/* Join the two lists l1 and l2, returning the combination. */
static ptrlst_nfa_states_t*
ptrlst_append(ptrlst_nfa_states_t *l1,
              ptrlst_nfa_states_t *l2) {
  ptrlst_nfa_states_t *old_l1 = l1;
  while(l1->next)
    l1 = l1->next;
  l1->next = l2;
  return old_l1;
}
//////////////////////////////////////////////////////////////

nfa_state_t *
postfix_to_nfa(const int16_t *lst,
               int len) {
  assert(lst);
  assert(len);

  nfa_state_t *tmp_st;
  frag_t e1, e2, e;
  frag_t *stack = malloc(sizeof(frag_t) * len);
  frag_t *sp = stack;

#define push(state) (*sp++ = state)
#define pop() (*--sp)

  for(int i = 0; i < len; ++i) {
    switch(lst[i]) {
      default:
        push(frag(tmp_st, ptrlst_constr(&tmp_st->out1)));
        break;
      case '.':
        tmp_st = nfa_state(SS_DOT, NULL, NULL);
        push(frag(tmp_st, ptrlst_constr(&tmp_st->out1)));
        break;
      case SS_CONCAT:	/* catenate */
        e2 = pop();
        e1 = pop();
        ptrlst_patch(e1.out, e2.start);
        push(frag(e1.start, e2.out));
        break;
      case '|':	/* alternate */
        e2 = pop();
        e1 = pop();
        tmp_st = nfa_state(SS_SPLIT, e1.start, e2.start);
        push(frag(tmp_st, ptrlst_append(e1.out, e2.out)));
        break;
      case '?':	/* zero or one */
        e = pop();
        tmp_st = nfa_state(SS_SPLIT, e.start, NULL);
        push(frag(tmp_st, ptrlst_append(e.out, ptrlst_constr(&tmp_st->out2))));
        break;
      case '*':	/* zero or more */
        e = pop();
        tmp_st = nfa_state(SS_SPLIT, e.start, NULL);
        ptrlst_patch(e.out, tmp_st);
        push(frag(tmp_st, ptrlst_constr(&tmp_st->out2)));
        break;
      case '+':	/* one or more */
        e = pop();
        tmp_st = nfa_state(SS_SPLIT, e.start, NULL);
        ptrlst_patch(e.out, tmp_st);
        push(frag(e.start, ptrlst_constr(&tmp_st->out2)));
        break;
      case SS_IGNORE:
        break;
    }
  }

  e = pop();
  tmp_st = nfa_state(SS_MATCH, NULL, NULL);
  ptrlst_patch(e.out, tmp_st);
  return e.start;
#undef push
#undef pop
}
///////////////////////////////////////////////////////

RegExp *
parseRegExp(const char *input) {
  int postfix_len, rc;
  int16_t *postfix_arr = NULL;

  rc = regex2postfix(input, &postfix_arr, &postfix_len, -1);
  do {
    if (rc) {
      printf("Shit happened\n");
      break;
    }

    for (int i = 0; i < postfix_len; ++i) {
      printf("%c\t", (char)postfix_arr[i]);
    }
    printf("\n");
  } while(0);

  free(postfix_arr);
  return NULL;
}
///////////////////////////////////////////////////////
