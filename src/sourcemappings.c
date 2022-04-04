#include "sourcemappings.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static void parse_node(const char *str,
                       const char *end,
                       const char **prev_node_fields[5],
                       int *prev_node_sizes[5]) {

  const char ***p_dst = prev_node_fields;
  int **p_dst_s = prev_node_sizes;
  char *tok_part = NULL;

  for (tok_part = strchr(str, ':');
       tok_part != NULL && tok_part < end;
       tok_part = strchr(str, ':'), ++p_dst, ++p_dst_s) {

    if (tok_part - str) {
      **p_dst_s = tok_part - str;
      **p_dst = str;
    }
    str = tok_part + 1;
  }

  if (end - str) {
    **p_dst_s = end - str;
    **p_dst = str;
  }
}
//////////////////////////////////////////////////////////////

cnode_t *
sm_decompile(const char *str, int *count) {
  int nn = 0;
  for (const char *c = str; *c; ++c) {
    if (*c == ';') ++nn;
  }
  if (!nn) return NULL;

  cnode_t *res = calloc(nn+1, sizeof(cnode_t));
  cnode_t *p_res = res;
  cnode_t prev = {0};
  char *tok_node;
  int nc = 0;

  const char **dst[5] = {&prev.s, &prev.l, &prev.f, &prev.j, &prev.m};
  int *dst_s[5] = {&prev.s_, &prev.l_, &prev.f_, &prev.j_, &prev.m_};

  for (; (tok_node = strchr(str, ';')) != NULL; ++nc) {
    parse_node(str, tok_node, dst, dst_s);
    *p_res++ = prev;
    str = tok_node + 1;
    while (*str == ';') {
      ++str;
      ++nc;
      *p_res++ = prev;
    };
  }

  if (strlen(str)) {
    parse_node(str, str + strlen(str), dst, dst_s);
    *p_res++ = prev;
    ++nc;
  }

  *count = nc;
  return res;
}
//////////////////////////////////////////////////////////////

void cnode_print(const cnode_t *n) {
  printf("%.*s:%.*s:%.*s:%.*s:%.*s\n", n->s_, n->s,
         n->l_, n->l, n->f_, n->f, n->j_, n->j, n->m_, n->m);
}
//////////////////////////////////////////////////////////////

void
sm_free(cnode_t *nodes) {
  free(nodes);
}
//////////////////////////////////////////////////////////////Я
