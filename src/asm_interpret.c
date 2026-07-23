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

// (a)+0 to convert types to int and remove const/volatile modifiers
#define MIN(a, b)                                                              \
  _Generic((a) + 0,                                                            \
      int: ((a) < (b) ? (a) : (b)),                                            \
      long: ((a) < (b) ? (a) : (b)),                                           \
      double: ((a) < (b) ? (a) : (b)),                                         \
      size_t: ((a) < (b) ? (a) : (b)),                                         \
      float: ((a) < (b) ? (a) : (b)))

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
      // we don't want to consume '\n' or '\0'
    }
    break;
  }
}
//////////////////////////////////////////////////////////////

token_t tkz_next_token(tokenizer_t *t) {
  tkz_skip_whitespaces_and_comments(t);

  size_t token_start = t->cursor;
  char c = t->source[t->cursor];
  token_type_t tt = TT_UNKNOWN;

  do {
    if (c == '\0') {
      // ++t->cursor;  don't want to consume '\0'
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

#define MAX_LBL_N 64
#define MAX_STACK_DEPTH 256

typedef struct sa_context {
  tokenizer_t tkz;
  uint64_t FLAGS;

  size_t RLP; // labels pointer
  token_t labels[MAX_LBL_N];

  size_t RSP;
  size_t stack[MAX_STACK_DEPTH];

  int32_t registers[128];
} sa_context_t;

static size_t sa_find_lbl(const sa_context_t *ctx, const token_t *lbl) {
  for (size_t i = 0; i < MAX_LBL_N; ++i) {
    if (sv_cmp(&ctx->labels[i].sv, &lbl->sv)) {
      continue;
    }
    return i;
  }
  return MAX_LBL_N;
}

static int sa_push_lbl(sa_context_t *ctx, const token_t *lbl) {
  if (ctx->RLP >= MAX_LBL_N) {
    return 1;
  }
  ctx->labels[ctx->RLP++] = *lbl;
  return 0;
}
//////////////////////////////////////////////////////////////

static int sa_mov(sa_context_t *ctx) {
  token_t ta_dst = tkz_next_token(&ctx->tkz);
  if (ta_dst.tt != TT_IDENTIFIER) {
    return -1;
  }
  token_t tc = tkz_next_token(&ctx->tkz);
  if (tc.tt != TT_COMMA) {
    return -2;
  }
  token_t ta_src = tkz_next_token(&ctx->tkz);
  if (ta_src.tt != TT_IDENTIFIER && ta_src.tt != TT_INT) {
    return -3;
  }

  ctx->registers[(int)ta_dst.sv.data[0]] =
      ta_src.tt == TT_INT ? atoi(ta_src.sv.data)
                          : ctx->registers[(int)ta_src.sv.data[0]];

  printf("mov %.*s %.*s %.*s", ta_dst.sv.len, ta_dst.sv.data, tc.sv.len,
         tc.sv.data, ta_src.sv.len, ta_src.sv.data);
  return 0;
}
//////////////////////////////////////////////////////////////

static int sa_inc(sa_context_t *ctx) {
  token_t ta = tkz_next_token(&ctx->tkz);
  if (ta.tt != TT_IDENTIFIER) {
    return -1;
  }
  printf("inc %.*s", ta.sv.len, ta.sv.data);
  ++ctx->registers[(int)ta.sv.data[0]];
  return 0;
}
//////////////////////////////////////////////////////////////

static int sa_dec(sa_context_t *ctx) {
  token_t ta = tkz_next_token(&ctx->tkz);
  if (ta.tt != TT_IDENTIFIER) {
    return -1;
  }
  printf("dec %.*s", ta.sv.len, ta.sv.data);
  --ctx->registers[(int)ta.sv.data[0]];
  return 0;
}
//////////////////////////////////////////////////////////////

typedef enum { ADD = 0, SUB, MUL, DIV } bin_arithm_op;

static int sa_bin_arithm_op(sa_context_t *ctx, bin_arithm_op op) {
  token_t ta_dst = tkz_next_token(&ctx->tkz);
  if (ta_dst.tt != TT_IDENTIFIER) {
    return -1;
  }
  token_t tc = tkz_next_token(&ctx->tkz);
  if (tc.tt != TT_COMMA) {
    return -2;
  }
  token_t ta_src = tkz_next_token(&ctx->tkz);
  if (ta_src.tt != TT_IDENTIFIER && ta_src.tt != TT_INT) {
    return -3;
  }

  printf(" %.*s %.*s %.*s", ta_dst.sv.len, ta_dst.sv.data, tc.sv.len,
         tc.sv.data, ta_src.sv.len, ta_src.sv.data);

  int32_t *dst = &ctx->registers[(int)ta_dst.sv.data[0]];
  int32_t src = ta_src.tt == TT_INT ? atoi(ta_src.sv.data)
                                    : ctx->registers[(int)ta_src.sv.data[0]];
  switch (op) {
  case ADD:
    *dst += src;
    break;
  case SUB:
    *dst -= src;
    break;
  case MUL:
    *dst *= src;
    break;
  case DIV:
    *dst /= src;
    break;
  }
  return 0;
}

static int sa_add(sa_context_t *ctx) {
  printf("add");
  return sa_bin_arithm_op(ctx, ADD);
}
//////////////////////////////////////////////////////////////

static int sa_sub(sa_context_t *ctx) {
  printf("sub");
  return sa_bin_arithm_op(ctx, SUB);
}
//////////////////////////////////////////////////////////////

static int sa_mul(sa_context_t *ctx) {
  printf("mul");
  return sa_bin_arithm_op(ctx, MUL);
}
//////////////////////////////////////////////////////////////

static int sa_div(sa_context_t *ctx) {
  printf("div");
  return sa_bin_arithm_op(ctx, DIV);
}
//////////////////////////////////////////////////////////////

static int sa_jmp(sa_context_t *ctx) {
  token_t tlbl = tkz_next_token(&ctx->tkz);
  if (tlbl.tt != TT_LABEL) {
    return 1;
  }
  return 0;
}
//////////////////////////////////////////////////////////////

typedef struct sa_cmd_handler {
  const char *cmd;
  int (*handler)(sa_context_t *);
} sa_cmd_handler_t;

static const sa_cmd_handler_t sa_cmd_handlers[] = {
    {.cmd = "mov", .handler = sa_mov}, {.cmd = "inc", .handler = sa_inc},
    {.cmd = "dec", .handler = sa_dec}, {.cmd = "add", .handler = sa_add},
    {.cmd = "sub", .handler = sa_sub}, {.cmd = "mul", .handler = sa_mul},
    {.cmd = "div", .handler = sa_div}, {.cmd = "jmp", .handler = sa_jmp},
    {.cmd = NULL, .handler = NULL},
};

static int sa_cmd_process(sa_context_t *ctx, token_t ct) {
  for (size_t i = 0; sa_cmd_handlers[i].cmd != NULL; ++i) {
    size_t n = MIN(strlen(sa_cmd_handlers[i].cmd), ct.sv.len);
    if (strncmp(sa_cmd_handlers[i].cmd, ct.sv.data, n)) {
      continue;
    }

    return sa_cmd_handlers[i].handler(ctx);
  }

  printf("UNKNOWN: (%.*s [%s:%d]) ", ct.sv.len, ct.sv.data,
         token_type_str(ct.tt), ct.sv.len);
  return 0;
}
//////////////////////////////////////////////////////////////

void simple_assembler(const char *program) {
  sa_context_t ctx = {0};
  ctx.tkz = tokenizer(program);
  token_t ct = tkz_next_token(&ctx.tkz);

  for (; ct.tt != TT_EOF; ct = tkz_next_token(&ctx.tkz)) {
    if (ct.tt == TT_NEWLINE) {
      printf("\n");
      continue;
    }

    if (ct.tt == TT_LABEL) {
      if (sa_find_lbl(&ctx, &ct) >= MAX_LBL_N) {
        sa_push_lbl(&ctx, &ct);
      }
      printf("label %.*s\n", ct.sv.len, ct.sv.data);
      continue;
    }
    sa_cmd_process(&ctx, ct);
  }
  printf("\n");
}
//////////////////////////////////////////////////////////////

int asm_interpret_main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  // clang-format off
  const char *program = 
    "mov   a, 81         ; value1\n"
    "mov   b, 153        ; value2\n"
    "call  init\n"
    "call  proc_gcd\n"
    "call  print\n"
    "end\n"
    "\n"
    "proc_gcd:\n"
    "    cmp   c, d\n"
    "    jne   loop\n"
    "    ret\n"
    "\n"
    "loop:\n"
    "    cmp   c, d\n"
    "    jg    a_bigger\n"
    "    jmp   b_bigger\n"
    "\n"
    "a_bigger:\n"
    "    sub   c, d\n"
    "    jmp   proc_gcd\n"
    "\n"
    "b_bigger:\n"
    "    sub   d, c\n"
    "    jmp   proc_gcd\n"
    "\n"
    "init:\n"
    "    cmp   a, 0\n"
    "    jl    a_abs\n"
    "    cmp   b, 0\n"
    "    jl    b_abs\n"
    "    mov   c, a            ; temp1\n"
    "    mov   d, b            ; temp2\n"
    "    ret\n"
    "\n"
    "a_abs:\n"
    "    mul   a, -1\n"
    "    jmp   init\n"
    "\n"
    "b_abs:\n"
    "    mul   b, -1\n"
    "    jmp   init\n"
    "\n"
    "print:\n"
    "    msg   'gcd(', a, ', ', b, ') = ', c\n"
    "    ret\n";
  // clang-format on
  simple_assembler(program);
  return 0;
}
//////////////////////////////////////////////////////////////
