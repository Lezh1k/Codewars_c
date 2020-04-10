#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "regexp.h"
nfa_state_t match_state = {.signal = SS_MATCH};

typedef struct paren {
  int nalt;
  int natom;
} paren_t;

static paren_t paren() {
  paren_t r = {.nalt = 0, .natom = 0};
  return r;
}
///////////////////////////////////////////////////////

int
regex2postfix(const char *regexp,
              int16_t **result,
              int *len) {
  static paren_t st_paren[0xff] = {0};
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
          return ERR_WRONG_REGEXP; //wrong regexp

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
      case '+':
      case '?':
        if(natom == 0)
          return ERR_WRONG_REGEXP; //wrong regexp
        *res++ = *regexp;
        break;

      default: //any character, except escape, ()|+?*
        if(natom > 1){
          --natom;
          *res++ = SS_CONCAT;
        }
        *res++ = *regexp;
        natom++;
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
///////////////////////////////////////////////////////
///////////////////////////////////////////////////////

nfa_state_t *nfa_state(int32_t signal,
                       nfa_state_t *out,
                       nfa_state_t *out1,
                       int32_t index) {
  nfa_state_t *res = malloc(sizeof(nfa_state_t));
  res->signal = signal;
  res->out = out;
  res->out1 = out1;
  res->index = index;
  return res;
}
///////////////////////////////////////////////////////

frag_t
frag(nfa_state_t *start,
     ptr_list_t *out) {
  frag_t res = {
    .start = start,
    .out = out
  };
  return res;
}
///////////////////////////////////////////////////////

ptr_list_t *
pl_list(nfa_state_t **outp) {
  ptr_list_t *l;
  l = (ptr_list_t *)outp;
  l->next = NULL;
  return l;
}
///////////////////////////////////////////////////////

void
pl_patch(ptr_list_t *lst,
         nfa_state_t *s) {
  ptr_list_t *next;
  for(;lst;lst=next){
    next=lst->next;
    lst->state=s;
  }
}
///////////////////////////////////////////////////////

ptr_list_t *
pl_append(ptr_list_t *l,
          ptr_list_t *r) {
  ptr_list_t *old_l = l;
  while(l->next) l = l->next;
  l->next = r;
  return old_l;
}
///////////////////////////////////////////////////////

nfa_state_t *regexp_postfix2nfa(const int16_t *lst,
                                int len) {

  if(!len)
    return NULL;

  frag_t e1, e2, e;
  nfa_state_t *s;
  //  frag_t *stack = malloc((size_t)len * sizeof (frag_t));
  frag_t stack[128] = {0};
  frag_t *sp = stack;
  int32_t ix = 0;

#define push(state) (*sp++ = state)
#define pop() (*--sp)

  for(int i = 0; i < len; ++i) {
    int p = lst[i];
    switch(p){
      default:
        s = nfa_state(p, NULL, NULL, ++ix);
        push(frag(s, pl_list(&s->out)));
        break;
      case '.':
        s = nfa_state(SS_DOT, NULL, NULL, ++ix);
        push(frag(s, pl_list(&s->out)));
        break;
      case SS_CONCAT:	/* catenate */
        e2 = pop();
        e1 = pop();
        pl_patch(e1.out, e2.start);
        push(frag(e1.start, e2.out));
        break;
      case '|':	/* alternate */
        e2 = pop();
        e1 = pop();
        s = nfa_state(SS_SPLIT, e1.start, e2.start, ++ix);
        push(frag(s, pl_append(e1.out, e2.out)));
        break;
      case '?':	/* zero or one */
        e = pop();
        s = nfa_state(SS_SPLIT, e.start, NULL, ++ix);
        push(frag(s, pl_append(e.out, pl_list(&s->out1))));
        break;
      case '*':	/* zero or more */
        e = pop();
        s = nfa_state(SS_SPLIT, e.start, NULL, ++ix);
        pl_patch(e.out, s);
        push(frag(s, pl_list(&s->out1)));
        break;
      case '+':	/* one or more */
        e = pop();
        s = nfa_state(SS_SPLIT, e.start, NULL, ++ix);
        pl_patch(e.out, s);
        push(frag(e.start, pl_list(&s->out1)));
        break;
    }
  }

  e = pop();
  if(sp != stack)
    return NULL;
  pl_patch(e.out, &match_state);
  return e.start;
#undef pop
#undef push
}
///////////////////////////////////////////////////////
