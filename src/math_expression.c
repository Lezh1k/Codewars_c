#include "math_expression.h"

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

// careful. types sorted by precedence
typedef enum {
  TT_UNDEFINED = 0,
  TT_OPER,
//binary operators
  TT_ADD,
  TT_SUB,
  TT_MUL,
  TT_DIV,
//unary operators
  TT_NEG,
//parenthesis
  TT_PAR_L,
  TT_PAR_R,
//numbers
  TT_NUM
} token_type_t;

static bool tt_is_operator(token_type_t tt) {
  return tt >= TT_ADD && tt <= TT_NEG; //see enum
}

static inline int tt_precedence(token_type_t tt) {
  return tt;
}

static inline const char *token_type_str(token_type_t tt) {
  static const char *strs[] = {
    "UND", "OPER", "ADD", "SUB",
    "MUL", "DIV", "NEG", "PAR_L", "PAR_R", "NUM"
  };
  return strs[tt];
}
//////////////////////////////////////////////////////////////

typedef struct token {
  const char *p_str;
  size_t len;
  token_type_t type;
} token_t;

static inline bool token_is_empty(const token_t *t) {
  return t->p_str == NULL;
}
//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

double calculate (const char *expression);
static token_t *tokenize(const char *expression);
static token_t *infix_to_postfix(const token_t *infix);
static double calculate_postfix(const token_t *postfix);

static bool is_operator(char c) {
  return c && strchr("+-/*", c);
}

static bool is_number(char c) {
  return c && strchr("0123456789.", c);
}

static bool is_paren(char c) {
  return c && strchr("()", c);
}

static token_type_t paren_type(char c) {
  return c == '(' ? TT_PAR_L : TT_PAR_R;
}
//////////////////////////////////////////////////////////////

double
calculate (const char *expression) {
  token_t *lst_tokens = tokenize(expression);
  if (!lst_tokens) {
    return 0.0;
  }

  printf("\n***************\nINFIX:\n");
  for (const token_t *t = lst_tokens; t && !token_is_empty(t); ++t) {
    printf("%.*s : %s\n", (int)t->len, t->p_str, token_type_str(t->type));
  }

  printf("\n***************\nPOSTFIX:\n");

  token_t *postfix = infix_to_postfix(lst_tokens);
  for (const token_t *t = postfix; t && !token_is_empty(t); ++t) {
    printf("%.*s ", (int)t->len, t->p_str);
  }
  double res = calculate_postfix(postfix);
  free(lst_tokens);
  free(postfix);
  return res;
}
//////////////////////////////////////////////////////////////

token_t*
tokenize(const char *expression) {
  size_t tn = 0;
  for (const char *c = expression; *c; ++c) {
    if (isspace(*c))
      continue;
    ++tn;
  }

  token_t *lst_tokens = calloc(tn+1, sizeof(token_t));
  if (!lst_tokens) {
    perror("failed to allocate memory for tokens");
    return NULL;
  }

  token_t *pt = lst_tokens;
  for (const char *c = expression; *c; ++c) {
    if (is_number(*c)) {
      if (token_is_empty(pt)) { // we here after several spaces or at the beginning of expression
        pt->p_str = c;
        pt->type = TT_NUM;
      }
      ++pt->len;
      continue;
    }

    if (!token_is_empty(pt))
      ++pt;

    if (isspace(*c))
      continue;

    if (is_operator(*c)) {
      pt->p_str = c;
      pt->len = 1;
      pt->type = TT_OPER;
      ++pt;
      continue;
    }

    if (is_paren(*c)) {
      pt->p_str = c;
      pt->len = 1;
      pt->type = paren_type(*c);
      ++pt;
      continue;
    }

    perror("undefined token\n");
    exit(1);
  }

  for (token_t *cur_tok = lst_tokens; !token_is_empty(cur_tok); ++cur_tok) {
    if (cur_tok->type != TT_OPER)
      continue;

    switch(*cur_tok->p_str) {
      case '+':
        cur_tok->type = TT_ADD;
        continue;
      case '*':
        cur_tok->type = TT_MUL;
        continue;
      case '/':
        cur_tok->type = TT_DIV;
        continue;
    }

    if (*cur_tok->p_str != '-') {
      printf("shit happened. undefined operator : %c\n", *cur_tok->p_str);
      exit(1);
    }

    token_t *prev_tok = cur_tok - 1;
    if (prev_tok < lst_tokens) { // expr begin
      cur_tok->type = TT_NEG;
      continue;
    }

    cur_tok->type = TT_NEG;
    if (prev_tok->type == TT_NUM ||
        *prev_tok->p_str == ')') {
      cur_tok->type = TT_SUB;
    }
  }

  return lst_tokens;
}
//////////////////////////////////////////////////////////////

token_t *
infix_to_postfix(const token_t *infix) {
  int len = 0;
  for (const token_t *t = infix; !token_is_empty(t); ++t, ++len);
  if (len == 0) return NULL;
  token_t *stack = malloc(sizeof(token_t) * (len+1)); //for '(' at the beginning
  token_t *sp = stack; //stack pointer

#define st_push(x) (*sp++ = x)
#define st_pop() (*--sp)
#define st_empty() (sp == stack)
#define st_peek() (sp-1)

  token_t *postfix = malloc(sizeof(token_t) * len);
  token_t *out = postfix;
  token_t start_tok = {.len = 1, .p_str = "(", .type = TT_PAR_L};

  st_push(start_tok);
  for (const token_t *t = infix; !token_is_empty(t); ++t) {
    if (t->type == TT_NUM) {
      *out++ = *t;
      continue;
    }

    if (t->type == TT_PAR_L) {
      st_push(*t);
      continue;
    }

    if (t->type == TT_PAR_R) {
      while (!st_empty() &&
             st_peek()->type != TT_PAR_L)
        *out++ = st_pop();
      st_pop(); //remove left parenthesis
      continue;
    }

    if (!tt_is_operator(t->type)) {
      printf("shit is happened %.*s\n", (int)t->len, t->p_str);
      exit(1);
    }

    while (!st_empty() &&
           tt_is_operator(st_peek()->type) &&
           tt_precedence(st_peek()->type) >= tt_precedence(t->type)) {
      *out++ = st_pop();
    }
    st_push(*t);
  }

  while (!st_empty() &&
         st_peek()->type != TT_PAR_L) {
    *out++ = st_pop();
  }

  free(stack);
  return postfix;

#undef st_push
#undef st_peek
#undef st_pop
}
//////////////////////////////////////////////////////////////

double calculate_postfix(const token_t *postfix) {
  double res = 0.0;
  for (const token_t *t = postfix; !token_is_empty(t); ++t) {

  }
  return res;
}
