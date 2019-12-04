#include "regexp.h"
#include <stdbool.h>
#include <stddef.h>

static const char* m_escapes_str = "&+\\()?*|^${}[]-.";
#define ESCAPES_LEN 16

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
              int **result,
              int *len) {
  static paren_t st_paren[0xff] = {0};
  paren_t *paren_sp = st_paren;
#define paren_push(x) (*paren_sp++ = x)
#define paren_pop() (*--paren_sp)
#define paren_empty() (paren_sp == st_paren)

  //n alternatives , n atoms
  //atom is any symbol except ()|*?+
  int nalt, natom;
  int *res = malloc(sizeof(int) * strlen(regexp)*2+1);
  *result = res;
  paren_t p = paren();
  //in curent parentheses
  nalt = natom = 0;
  for(;*regexp;regexp++){
    switch(*regexp){
      case '.' :        
      default: //any character, except escape, ()|+?*        
        if(natom > 1){
          --natom;
          *res++ = SS_CONCAT;
        }
        *res++ = *regexp;
        natom++;
        break;

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
////////////////////////////////////////////////////////////////////////////
