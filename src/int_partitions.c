#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include "int_partitions.h"

///////////////////////////////////////////////////////
typedef struct node {
  int32_t val;
  struct node *next;
} node_t;

typedef struct slist {
  node_t *root;
  uint32_t count;
} slist_t;
///////////////////////////////////////////////////////

static slist_t slst_new() {
  node_t *root = malloc(sizeof(node_t));
  slist_t lst ;
  root->next = NULL;
  lst.count = 0;
  lst.root = root;
  return lst;
}
///////////////////////////////////////////////////////

static node_t* slst_search_ins_place(slist_t *lst, int32_t val) {
  node_t *rn = lst->root;
  for (; rn->next; rn=rn->next) {
    if (rn->next->val < val)
      continue;
    return rn->next->val == val ? NULL : rn;
  }
  return rn;
}
///////////////////////////////////////////////////////

static void slst_push(slist_t *lst, int32_t it) {
  node_t *ni;
  node_t *ip = slst_search_ins_place(lst, it);
  if (!ip)
    return;

  ++lst->count;
  ni = malloc(sizeof(node_t));
  if (ni == NULL) {
    printf("Couldn't allocate memory.");
    exit(1);
  }

  ni->val = it;
  ni->next = ip->next;
  ip->next = ni;
}
///////////////////////////////////////////////////////

static void slst_free(slist_t *lst) {
  node_t *tmp = lst->root;
  while(tmp) {
    tmp = tmp->next;
    free(lst->root);
    lst->root = tmp;
  }
}
///////////////////////////////////////////////////////

static double slst_mean(slist_t *lst) {
  node_t *tmp = lst->root->next;
  int64_t sum = 0;
  while(tmp) {
    sum += tmp->val;
    tmp = tmp->next;
  }
  return ((double) sum) / ((double) lst->count);
}
///////////////////////////////////////////////////////

static double slst_median(slist_t *lst) {
  double median = 0.0;
  int size1, size2, pos = 0;
  node_t *tmp = lst->root->next;

  size1 = lst->count / 2;
  size2 = size1 - (lst->count & 1 ? 0:1);

  while(tmp) {
    if (pos == size1)
      median += tmp->val;
    if (pos == size2)
      median += tmp->val;

    tmp = tmp->next;
    ++pos;
  }
  return median / 2.0;
}
///////////////////////////////////////////////////////

static void slst_print(slist_t *lst) {
  node_t *tmp = lst->root->next;
  printf("searched list:\n");
  while(tmp) {
    printf("%d ", tmp->val);
    tmp = tmp->next;
  }
  printf("\n");
}
///////////////////////////////////////////////////////

static void decompositions(int n, int k, int i, slist_t *lst);
static int64_t product_max(int64_t n);
static int64_t powi(int64_t base, uint64_t exp);

#define ADDENDS_COUNT 1024
static int32_t addends[ADDENDS_COUNT] = {0};

char *part(int n) {
  assert(n > 0);
  char *buff = malloc(128);
  char *buff_it = buff;
  int64_t max;
  slist_t lst = slst_new();

  memset(addends, 0, ADDENDS_COUNT * sizeof(int32_t));
  decompositions(n, n, 0, &lst);
  max = product_max(n); //range
  buff_it += sprintf(buff_it, "Range: %"PRId64" ", max-1);
  buff_it += sprintf(buff_it, "Average: %.02f ", slst_mean(&lst));
  buff_it += sprintf(buff_it, "Median: %.02f", slst_median(&lst));
  slst_free(&lst);
  return buff;
}
///////////////////////////////////////////////////////

int64_t product_max(int64_t n) {
  if (n <= 4)
    return n;
  switch(n % 3) {
    case 0:
      return powi(3, n/3);
    case 1:
      return powi(3, (n-4)/3) * 4;
    case 2:
      return powi(3, (n-2)/3) * 2;
  }
  assert(false);
  return 0;
}
///////////////////////////////////////////////////////

int64_t powi (int64_t base, uint64_t exp) {
  int64_t res = 1;
  while (exp) {
    if (exp & 1)
      res *= base;
    exp >>= 1;
    base *= base;
  }
  return res;
}
///////////////////////////////////////////////////////

void decompositions(int n, int k, int i, slist_t *lst) {
  if ( n < 0 )
    return;

  if ( n == 0 ) {
    int j, mul = 1;
    for (j = 0; j < i; j++) {
      mul *= addends[j];
    }
    slst_push(lst, mul);
  } else {
    if ( n - k >= 0) {
      addends[i] = k; // фиксируем i-ое слагаемое
      decompositions(n - k, k, i + 1, lst);
    }
    if ( k - 1 > 0) {
      decompositions(n, k - 1, i, lst);
    }
  }
  return;
}
///////////////////////////////////////////////////////
