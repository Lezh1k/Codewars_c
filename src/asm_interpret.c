#include "asm_interpret.h"

#include <assert.h>
#include <ctype.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct str_view {
  const char *data;
  size_t len;
} str_view_t;
#define SV_FMT "%.*s"

str_view_t sv(const char *str, size_t len) {
  assert(str);
  return (str_view_t){
      .data = str,
      .len = len,
  };
}

str_view_t sv_from_str(const char *data) {
  assert(data);
  return (str_view_t){
      .data = data,
      .len = strlen(data),
  };
}

int sv_cmp(const str_view_t *l, const str_view_t *r) {
  size_t len = l->len >= r->len ? r->len : l->len;
  for (size_t i = 0; i < len; ++i) {
    if (l->data[i] == r->data[i])
      continue;
    return (int)(l->data[i] - r->data[i]);
  }
  return 0;
}
//////////////////////////////////////////////////////////////

// 1. Define Token Types
typedef enum {
  TT_EOF = 0,
  TT_LABEL,      // "main:"
  TT_IDENTIFIER, // "mov", "eax"
  TT_DIRECTIVE,  // ".global", ".data"
  TT_INT,        // "42", "0x1A"
  TT_COMMA,      // ','
  TT_NEWLINE,    // '\n'
  TT_STR,        // strings
  TT_UNKNOWN
} token_type_t;

static const char *token_type_str(token_type_t tt) {
  static const char *tt_tbl[] = {"EOF",       "LABEL",  "IDENTIFIER",
                                 "DIRECTIVE", "INT",    "COMMA",
                                 "NELINE",    "STRING", "UNKNOWN"};
  return tt_tbl[tt];
}

typedef struct token {
  str_view_t sv;
  token_type_t tt;
} token_t;

typedef struct tokenizer {
  const char *source;
  size_t cursor;
} tokenizer_t;

tokenizer_t tokenizer(const char *src) {
  tokenizer_t t = {
      .source = src,
      .cursor = 0,
  };
  return t;
}

void tkz_skip_whitespaces_and_comments(tokenizer_t *t) {
  // loop to skip whitespaces and comments
  char c;
  while ((c = t->source[t->cursor]) != '\0') {
    if (isspace(c) && c != '\n') {
      ++t->cursor;
      continue;
    }

    if (c == ';' || c == '#') {
      // skip comment
      while (t->source[t->cursor] != '\n' && t->source[t->cursor] != '\0') {
        ++t->cursor;
        continue;
      }
      ++t->cursor; // consume '\n' or '\0' (we don't want it at the beginning of
                   // token)
    }
    break;
  }
}

token_t tkz_next_token(tokenizer_t *t) {
  tkz_skip_whitespaces_and_comments(t);

  size_t token_start = t->cursor;
  char c = t->source[t->cursor];
  token_type_t tt = TT_UNKNOWN;

  do {
    if (c == '\0') {
      ++t->cursor; // consume end of string
      tt = TT_EOF;
      break;
    }

    if (c == '\n') {
      ++t->cursor; // consume new line
      tt = TT_NEWLINE;
      break;
    }

    if (c == ',') {
      ++t->cursor; // consume comma
      tt = TT_COMMA;
      break;
    }

    if (c == '.') {
      while (t->source[t->cursor] == '\0' &&
             (isalnum(t->source[t->cursor]) || t->source[t->cursor] == '_')) {
        ++t->cursor;
      }
      tt = TT_DIRECTIVE;
      break;
    }

    if (isdigit(c) || (c == '-' && isdigit(t->source[t->cursor + 1]))) {
      ++t->cursor; // consume first digit or minus
      while (t->source[t->cursor] != '\0' &&
             (isxdigit(t->source[t->cursor]) || t->source[t->cursor] == 'x' ||
              t->source[t->cursor] == 'X')) {
        ++t->cursor;
      }
      tt = TT_INT;
      break;
    }

    if (c == '\'') {
      ++t->cursor; // consume open quotetion mark
      while (t->source[t->cursor] != '\0' && t->source[t->cursor] != '\'') {
        ++t->cursor;
      }
      ++t->cursor; // consume close quotetion mark
      tt = TT_STR;
      break;
    }

    // IDENTIFIERS, REGISTERS, INSTRUCTIONS and LABELS
    if (isalpha(c) || c == '_' || c == '$') {
      ++t->cursor;
      while (t->source[t->cursor] != '\0' &&
             (isalnum(t->source[t->cursor]) || t->source[t->cursor] == '_' ||
              t->source[t->cursor] == '$')) {
        ++t->cursor;
      }

      if (t->source[t->cursor] == ':') {
        ++t->cursor; // consume ':' at the end of label
        tt = TT_LABEL;
        break;
      }

      tt = TT_IDENTIFIER;
      break;
    }
  } while (0); // get type and token

  str_view_t sv = {
      .data = &t->source[token_start],
      .len = t->cursor - token_start,
  };

  token_t rt = {
      .sv = sv,
      .tt = tt,
  };

  return rt;
}
//////////////////////////////////////////////////////////////

typedef struct cmd {
  str_view_t sv;
  size_t addr;
} cmd_t;

typedef struct interpreter_context {
  // probably that's enough
  size_t EIP; // instruction pointer
  size_t ESP; // stack pointer

  cmd_t stack[256];
} interpreter_context_t;

void simple_assembler(const char *program) {
  tokenizer_t tkz = tokenizer(program);
  token_t ct = tkz_next_token(&tkz);
  for (; ct.tt != TT_EOF; ct = tkz_next_token(&tkz)) {
    if (ct.tt == TT_NEWLINE) {
      printf("\n");
      continue;
    }

    printf("(%.*s [%s:%d]) ", ct.sv.len, ct.sv.data, token_type_str(ct.tt),
           ct.sv.len);
  }
  printf("\n");
}
//////////////////////////////////////////////////////////////

int asm_interpret_main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  int registers[128] = {0};
  // clang-format off
  const char *program = 
    "; My first program\n"
    "mov  a, 5\n"
    "inc  a\n"
    "call function\n"
    "msg  '(5+1)/2 = ', a    ; output message\n"
    "end\n"
    "function:\n"
    "    div  a, 2\n"
    "    ret\n";
  // clang-format on
  simple_assembler(program);
  return 0;
}
//////////////////////////////////////////////////////////////
