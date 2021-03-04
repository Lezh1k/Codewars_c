#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "list.h"

static int
train_crash(const char *track,
            const char *a_train,
            int a_train_pos,
            const char *b_train,
            int b_train_pos,
            int limit) ;

typedef struct test {
  list_node_t node;
  char data;
} test_t;

static void print_test_it(list_node_t *it) {
  test_t *ptr = (test_t*)it;
  printf("%c\n", ptr->data);
}

int
main(int argc,
     char *argv[]) {
  printf("argc: %d\n", argc);
  for (int i = 0; i < argc; ++i)
    printf("argv[i]=\t%s\n", argv[i]);



  ///////////////////////////////////////////////////////  

  test_t t1, t2, t3;
  t1.data = '1';
  t2.data = '2';
  t3.data = '3';
  lst_init((list_node_t*)&t1);
  lst_init((list_node_t*)&t2);
  lst_init((list_node_t*)&t3);

  lst_push_back((list_node_t*) &t3,
                (list_node_t*) &t1);

  lst_push_back((list_node_t*) &t2,
                (list_node_t*) &t1);

  lst_for_each((list_node_t*) &t1,
               print_test_it);

  return 0;

  ///////////////////////////////////////////////////////
  const char *example_track =
      "                                /------------\\\n"
        "/-------------\\                /             |\n"
        "|             |               /              S\n"
        "|             |              /               |\n"
        "|        /----+--------------+------\\        |   \n"
        "\\       /     |              |      |        |     \n"
        " \\      |     \\              |      |        |                    \n"
        " |      |      \\-------------+------+--------+---\\\n"
        " |      |                    |      |        |   |\n"
        " \\------+--------------------+------/        /   |\n"
        "        |                    |              /    | \n"
        "        \\------S-------------+-------------/     |\n"
        "                             |                   |\n"
        "/-------------\\              |                   |\n"
        "|             |              |             /-----+----\\\n"
        "|             |              |             |     |     \\\n"
        "\\-------------+--------------+-----S-------+-----/      \\\n"
        "              |              |             |             \\\n"
        "              |              |             |             |\n"
        "              |              \\-------------+-------------/\n"
        "              |                            |               \n"
        "              \\----------------------------/ \n";

  train_crash(example_track, "Aaaa", 147, "Bbbbbbbbbbb", 288, 1000);
}

int
train_crash(const char *track,
            const char *a_train,
            int a_train_pos,
            const char *b_train,
            int b_train_pos,
            int limit) {
  printf("%s\n", track);
  return 42;
}
