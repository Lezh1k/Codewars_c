#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include "list.h"


typedef struct test {
  list_node_t node;
  int data;
} test_t;

static void print_test_it(list_node_t *it) {
  test_t *ptr = (test_t*)it;
  printf("%d\n", (int)ptr->data);
}

static list_node_t *
arr_to_lst(const test_t *arr,
           size_t len) {
  assert(len);
  assert(arr);

  list_node_t *head = (list_node_t*)arr++;
  lst_init((list_node_t*)head);
  while (--len) {
    lst_push_back((list_node_t*) head,
                  (list_node_t*) arr++);
  }
  return head;
}
///////////////////////////////////////////////////////

static list_node_t *
data_arr_to_lst(const int *in, size_t len) {
  test_t *lst = malloc(sizeof (test_t) * len);
  for (size_t i = 0; i < len; ++i) {
    lst[i].data = in[i];
  }
  return arr_to_lst(lst, len);
}
///////////////////////////////////////////////////////

/*
if (!headA || !headB) return NULL;
ListNode *a = headA, *b = headB;
while (a != b) {
  a = !a ? headB : a->next;
  b = !b ? headA : b->next;
}
return a;
*/

int
main(int argc,
     char *argv[]) {
  printf("argc: %d\n", argc);
  for (int i = 0; i < argc; ++i)
    printf("argv[i]=\t%s\n", argv[i]);

  int a[] = {4,1};
  int b[] = {5,6,1};
  int common[] = {8,4,1,5};

  list_node_t *lst_a = data_arr_to_lst(a, sizeof(a) / sizeof(*a));
  list_node_t *lst_b = data_arr_to_lst(b, sizeof(b) / sizeof(*b));
  list_node_t *lst_c = data_arr_to_lst(common, sizeof(common) / sizeof(*common));

  lst_a->prev->next = lst_c;
  lst_b->prev->next = lst_c;
  lst_c->prev->next = NULL;

  list_node_t *ta, *tb;
  ta = lst_a;
  tb = lst_b;

  while (ta != tb) {
    ta = !ta ? lst_b : ta->next;
    tb = !tb ? lst_a : tb->next;
  }

  printf("%p == %p\n", ta, lst_c);
  return 0;
}
