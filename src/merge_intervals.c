#include "merge_intervals.h"
static int cmp_interval_asc(const void *l, const void *r);

int
cmp_interval_asc(const void *l,
                 const void *r) {
  const interval_t *li, *ri;
  li = (const interval_t*)l;
  ri = (const interval_t*)r;
//  return li->first - ri->first;
  return (li->first > ri->first) - (li->first < ri->first);
}
///////////////////////////////////////////////////////

int
sum_intervals(const interval_t *v,
              size_t n) {
  if (n == 0)
    return 0;
  //it's better to implement own stack or use from library.
  interval_t *stack = malloc(sizeof (interval_t) * n);
  interval_t *sp = stack;

#define st_push(x) (*sp++ = x)
#define st_pop() (*--sp)
#define st_empty() (sp == stack)
#define st_top() (*(sp-1))

  qsort((void*)v, n, sizeof(interval_t), cmp_interval_asc);
  st_push(v[0]);
  for (size_t i = 0; i < n; ++i)
     printf("%d:%d\n", v[i].first, v[i].second);

  for (size_t i = 1; i < n; ++i) {
    interval_t top = st_top();
    if (top.second < v[i].first) {
      st_push(v[i]);
    } else if (top.second < v[i].second) {
      // Otherwise update the ending time of top if ending of current
      // interval is more
      top.second = v[i].second;
      st_pop();
      st_push(top);
    }
  }

  int sum = 0;
  while (!st_empty()) {
    interval_t top = st_pop();
    printf("%d:%d\n", top.first, top.second);
    sum += top.second - top.first;
  }
  free(stack);
  return sum;
}
///////////////////////////////////////////////////////
