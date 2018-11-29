#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#define ALP_POWER 8
#define NOPP -1
#define NOPP2 -2
#define BRSTART 4
#define INDENT_LEN 2

static char *ERR_MSG = "Error!";
typedef struct context {
  char *ct; //current token
  char *dst;
  int indent_level;
} context_t;


typedef int (*lpTokenHandler)(context_t *ctx);
static int thandler_plus(context_t *ctx);
static int thandler_coma(context_t *ctx);
static int thandler_minus(context_t *ctx);
static int thandler_point(context_t *ctx);
static int thandler_larr(context_t *ctx);
static int thandler_rarr(context_t *ctx);
static int thandler_lbr(context_t *ctx);
static int thandler_rbr(context_t *ctx);
static void add_indent(context_t *ctx);

/*format declarations*/
static const char *plus_fmt = "*p += %d;\n";
static const char *coma_fmt = "*p = getchar();\n";
static const char *minus_fmt = "*p -= %d;\n";
static const char *point_fmt = "putchar(*p);\n";
static const char *larr_fmt = "p -= %d;\n";
static const char *rarr_fmt = "p += %d;\n";
static const char *lbr_fmt = "if (*p) do {\n";
static const char *rbr_fmt = "} while (*p);\n";
/*^^^*/

static char alphabet[] = "+,-.<>[]\0";
typedef enum token {
  tplus = 0, tcoma = 1,
  tminus = 2, tpoint = 3,
  tlarr = 4, trarr = 5,
  tlbr = 6, trbr = 7
} token_t;

static int8_t alp_opposites[ALP_POWER] = {2, NOPP, 0, NOPP, 5, 4, NOPP, 6}; //we don't need to compensate ][, so for [ opposite is NOPP
static int8_t brace_increments[ALP_POWER] = {0, 0, 0, 0, 0, 0, 1, -1};
static int8_t res_lens[ALP_POWER] = {11, 16, 11, 13, 10, 10, 13, 14}; //WARNING! 0, 2, 4, 5 should be calculated. I took len(%d) = 3 (see format declarations), but it's NOT RIGHT!

static lpTokenHandler ctx_handlers[ALP_POWER] = {
  thandler_plus, thandler_coma,
  thandler_minus, thandler_point,
  thandler_larr, thandler_rarr,
  thandler_lbr, thandler_rbr,
};

static int filter_input(char *source, int len);
static int index_in_alp(char c);

char* brainfuck_to_c (const char* source) {
  register int aux = strlen(source);
  register char *src = malloc(aux+1);
  register char *dst = NULL;
  register int dstl, il;
  context_t ctx;

  dstl = il = 0;
  ctx.indent_level = 0;
  ctx.ct = src;

  if (!src) {
    printf("malloc failed\n");
    return ERR_MSG;
  }

  memcpy(src, source, aux);
  src[aux]=0;
  if (!filter_input(src, aux)) {
    free(src);
    return ERR_MSG;
  }

  for (aux = 0; src[aux] != NOPP; ++aux) {
    dstl += il*INDENT_LEN + res_lens[(int8_t)src[aux]];
    il += brace_increments[(int8_t)src[aux]];
  }
  ++dstl; //for 0 at the end

  dst = malloc(dstl);
  ctx.dst = dst;
  for (; *ctx.ct != NOPP; ++ctx.ct)
    ctx_handlers[(int)*ctx.ct](&ctx);

  *ctx.dst=0;

  free(src);
  return dst;
}
///////////////////////////////////////////////////////////

int filter_input(char *source, int len) {
  register int8_t *stack = malloc(len+1);
  register int8_t *sp = stack+1;
  register char *cur, *out ;
  register int aix, br = 0;
  cur = out = source;
  stack[0] = NOPP2;
  for (; *cur; ++cur) {
    if ((aix = index_in_alp(*cur)) == ALP_POWER)
      continue;
    *out++ = (char)aix;
    *sp++ = aix; //push

    br += brace_increments[aix];
    if (br < 0)
      return 0;

    if (alp_opposites[aix] == NOPP)
      continue;

    if (alp_opposites[aix] == *(sp-2)) { //sp-2 - almost peek. second item in stack.
      out -= 2;
      sp -= 2; //pop(), pop()
    }
  }
  *out++ = NOPP;
  free(stack);
  return !br;
}
///////////////////////////////////////////////////////////

int index_in_alp(char c) {
  register int ix = 0;
  if (c < alphabet[0]) return ALP_POWER;
  if (c > alphabet[ALP_POWER-1]) return ALP_POWER;
  alphabet[ALP_POWER] = c;
  for (; alphabet[ix] != c; ++ix) ;
  return ix;
}
///////////////////////////////////////////////////////////

void add_indent(context_t *ctx) {
  register int count;
  for (count = 0; count < ctx->indent_level*2; ++count)
    *ctx->dst++ = ' ';
}
///////////////////////////////////////////////////////

int thandler_plus(context_t *ctx) {
  register int count = 1;
  add_indent(ctx);
  for (; *(ctx->ct+1) == tplus; ++ctx->ct)
    ++count;
  ctx->dst += sprintf(ctx->dst, plus_fmt, count);
  *ctx->dst=0;
  return 0;
}
///////////////////////////////////////////////////////////

int thandler_coma(context_t *ctx){
  add_indent(ctx);
  ctx->dst += sprintf(ctx->dst, coma_fmt);
  *ctx->dst=0;
  return 0;
}
///////////////////////////////////////////////////////////

int thandler_minus(context_t *ctx){
  register int count = 1;
  add_indent(ctx);
  for (; *(ctx->ct+1) == tminus; ++ctx->ct, ++count)
    ;
  ctx->dst += sprintf(ctx->dst, minus_fmt, count);
  *ctx->dst=0;
  return 0;
}
///////////////////////////////////////////////////////////

int thandler_point(context_t *ctx){
  add_indent(ctx);
  ctx->dst += sprintf(ctx->dst, point_fmt);
  *ctx->dst=0;
  return 0;
}
///////////////////////////////////////////////////////////

int thandler_larr(context_t *ctx){
  register int count = 1;
  add_indent(ctx);
  for (; *(ctx->ct+1) == tlarr; ++ctx->ct, ++count)
    ;
  ctx->dst += sprintf(ctx->dst, larr_fmt, count);
  *ctx->dst=0;
  return 0;
}
///////////////////////////////////////////////////////////

int thandler_rarr(context_t *ctx){
  register int count = 1;
  add_indent(ctx);
  for (; *(ctx->ct+1) == trarr; ++ctx->ct, ++count)
    ;
  ctx->dst += sprintf(ctx->dst, rarr_fmt, count);
  *ctx->dst=0;
  return 0;
}
///////////////////////////////////////////////////////////

int thandler_lbr(context_t *ctx){
  add_indent(ctx);
  ++ctx->indent_level;
  ctx->dst += sprintf(ctx->dst, lbr_fmt);
  *ctx->dst=0;
  return 0;
}
///////////////////////////////////////////////////////////

int thandler_rbr(context_t *ctx){
  --ctx->indent_level;
  add_indent(ctx);
  ctx->dst += sprintf(ctx->dst, rbr_fmt);
  *ctx->dst=0;
  return 0;
}
///////////////////////////////////////////////////////////
