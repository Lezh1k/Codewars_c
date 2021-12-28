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

static RegExp* postfixToRegExp(const int16_t *lst,
                               int len);

int
regex2postfix(const char *regexp,
              int16_t **result,
              int *len) {
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

RegExp* postfixToRegExp(const int16_t *lst,
                        int len) {
  if(len <= 0)
    return NULL;

  RegExp **stack = malloc(sizeof(RegExp*) * (size_t)len);
  RegExp **sp = stack;
  RegExp *e2, *e1;

#define push(state) (*sp++ = state)
#define pop() (*--sp)

  for(int i = 0; i < len; ++i) {
    int p = lst[i];
    switch(p){
      default:
        push(normal((char)p));
        break;
      case '.':
        push(any());
        break;
      case SS_CONCAT:	/* catenate */
        e2 = pop();
        e1 = pop();
        push(add(e1, e2));
        break;
      case '|':	/* alternate */
        e2 = pop();
        e1 = pop();
        push(orfn(e1, e2));
        break;
      case '?':	/* zero or one */
        e1 = pop();
        //push(zeroOrOne(e1)); not implemented in this kata
        break;
      case '*':	/* zero or more */
        e1 = pop();
        push(zeroOrMore(e1));
        break;
      case '+':	/* one or more */
        e1 = pop();
        //push(oneOrMore(e1)); not implemented in this kata
        break;
    }
  }

  e1 = pop();
  if(sp != stack)
    return NULL; // something went wrong
  return e1;
#undef pop
#undef push
}
///////////////////////////////////////////////////////

RegExp *
parseRegExp(const char *input) {
  int postfix_len, rc;
  int16_t *postfix_arr = NULL;

  rc = regex2postfix(input, &postfix_arr, &postfix_len);
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
