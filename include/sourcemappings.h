#ifndef SOURCEMAPPINGS_H
#define SOURCEMAPPINGS_H

#include <stdint.h>

typedef struct cnode {
  const char *s, *l, *f, *j, *m;
  int32_t s_, l_, f_, j_, m_;
} cnode_t;

void cnode_print(const cnode_t *n);
cnode_t* sm_decompile(const char *src, int *count);
void sm_free(cnode_t *nodes);

#endif // SOURCEMAPPINGS_H
