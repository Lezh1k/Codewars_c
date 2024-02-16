#include "bin_heap.h"
#include <stdio.h>

typedef struct timer_event {
  bh_node_t bh_node;
  uint32_t dst_tick;
} timer_event_t;

int8_t cmp_te(const bh_node_t *l, const bh_node_t *r) {
  timer_event_t *te_l = (timer_event_t *)l;
  timer_event_t *te_r = (timer_event_t *)r;
  if (te_l->dst_tick < te_r->dst_tick)
    return -1;
  return 1;
}

void print_te(const bh_node_t *node) {
  const timer_event_t *pte = (const timer_event_t *)node;
  printf("%d", pte->dst_tick);
}
//////////////////////////////////////////////////////////////

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  bheap_t *bh = bheap(15, cmp_te);
  timer_event_t tes[25] = {0};
  for (size_t i = 0; i < sizeof(tes) / sizeof(timer_event_t); ++i) {
    tes[i].dst_tick = i;
    bh_insert(bh, (bh_node_t *)&tes[i]);
  }
  bh_print(bh, print_te);
  printf("************\n");

  bh_free(bh);
  return 0;
}
//////////////////////////////////////////////////////////////
