#include <stdatomic.h>
#include <stdlib.h>

#include "list.h"
// Circular doubly linked list implementation.

#ifdef __clang__
  #define typeof(x) __typeof__(x)
#endif

#define barrier() __asm__ __volatile__("": : :"memory")

static __always_inline void __read_once_size(const volatile void *p, void *res, int size)
{
  switch (size) {
  case 1: *(uint8_t  *) res = *(volatile uint8_t  *) p; break;
  case 2: *(uint16_t *) res = *(volatile uint16_t *) p; break;
  case 4: *(uint32_t *) res = *(volatile uint32_t *) p; break;
  case 8: *(uint64_t *) res = *(volatile uint64_t *) p; break;
  default:
    barrier();
    __builtin_memcpy((void *)res, (const void *)p, size);
    barrier();
  }
}

static __always_inline void __write_once_size(volatile void *p, void *res, int size)
{
  switch (size) {
  case 1: *(volatile  uint8_t *) p = *(uint8_t  *) res; break;
  case 2: *(volatile uint16_t *) p = *(uint16_t *) res; break;
  case 4: *(volatile uint32_t *) p = *(uint32_t *) res; break;
  case 8: *(volatile uint64_t *) p = *(uint64_t *) res; break;
  default:
    barrier();
    __builtin_memcpy((void *)p, (const void *)res, size);
    barrier();
  }
}

/*
 * Prevent the compiler from merging or refetching reads or writes. The
 * compiler is also forbidden from reordering successive instances of
 * READ_ONCE and WRITE_ONCE, but only when the compiler is aware of some
 * particular ordering. One way to make the compiler aware of ordering is to
 * put the two invocations of READ_ONCE or WRITE_ONCE in different C
 * statements.
 *
 * These two macros will also work on aggregate data types like structs or
 * unions. If the size of the accessed data type exceeds the word size of
 * the machine (e.g., 32 bits or 64 bits) READ_ONCE() and WRITE_ONCE() will
 * fall back to memcpy and print a compile-time warning.
 *
 * Their two major use cases are: (1) Mediating communication between
 * process-level code and irq/NMI handlers, all running on the same CPU,
 * and (2) Ensuring that the compiler does not fold, spindle, or otherwise
 * mutilate accesses that either do not require ordering or that interact
 * with an explicit memory barrier or atomic instruction that provides the
 * required ordering.
 */

#define READ_ONCE(x)					\
({							\
  union { typeof(x) __val; char __c[1]; } __u =	\
    { .__c = { 0 } };			\
  __read_once_size(&(x), __u.__c, sizeof(x));	\
  __u.__val;					\
})

#define WRITE_ONCE(x, val)				\
({							\
  union { typeof(x) __val; char __c[1]; } __u =	\
    { .__val = (val) }; 			\
  __write_once_size(&(x), __u.__c, sizeof(x));	\
  __u.__val;					\
})


static void lst_insert(list_node_t *ni,
                       list_node_t *prev,
                       list_node_t *next);

void
lst_init(list_node_t *lst) {
  lst->next = lst->prev = lst;
}
///////////////////////////////////////////////////////

void
lst_push_back(list_node_t *ni,
              list_node_t *lst) {
  lst_insert(ni, lst->prev, lst);
}
///////////////////////////////////////////////////////

void
lst_push_front(list_node_t *ni,
               list_node_t *lst) {
  lst_insert(ni, lst, lst->next);
}
///////////////////////////////////////////////////////

void lst_insert(list_node_t *ni,
                list_node_t *prev,
                list_node_t *next) {
  next->prev = ni;
  ni->next = next;
  ni->prev = prev;
  WRITE_ONCE(prev->next, ni);
}
///////////////////////////////////////////////////////

bool
lst_empty(list_node_t *lst) {
  return READ_ONCE(lst->next) == lst;
}
///////////////////////////////////////////////////////

void
lst_del_entry(list_node_t *entry) {
  lst_del(entry->prev, entry->next);
  entry->prev = entry->next = LST_ENTRY_INVALID;
}
///////////////////////////////////////////////////////

void
lst_del(list_node_t *prev,
        list_node_t *next) {
  next->prev = prev;
  WRITE_ONCE(prev->next, next);
}

void
lst_for_each(list_node_t *head,
             void (*handler)(list_node_t *)) {
  list_node_t *t = head;
  do {
    handler(t);
    t = t->next;
  } while (t != head);
}
