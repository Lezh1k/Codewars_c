#ifndef LIST_H
#define LIST_H

#include <stdint.h>
#include <stdbool.h>

typedef struct list_node list_node_t;
struct list_node {
  list_node_t *next, *prev;
};
///////////////////////////////////////////////////////

#define LST_ENTRY_INVALID 0

void lst_init(list_node_t *lst);
void lst_push_back(list_node_t *lst, list_node_t *ni);
void lst_push_front(list_node_t *ni,
                    list_node_t *lst);

void lst_del(list_node_t *prev, list_node_t *next);
void lst_del_entry(list_node_t *entry);
bool lst_empty(list_node_t *lst);

void lst_for_each(list_node_t *head,
                  void (*handler)(list_node_t *entry));

#endif // LIST_H
