#include <assert.h>
#include <stdio.h>
#include "voronoi.h"

#define EPS 1.0e-9
static void line_norm(line_t *l);

line_t line_perpendicular(const line_t *l, const point_t *p) {
  line_t r;
  //A(y-y1)-B(x-x1)=0
  //Ax+By+C=0
  r.a = -l->b;
  r.b = l->a;
  r.c = l->b*p->x - l->a*p->y;
  return r;
}
///////////////////////////////////////////////////////

line_t line_from_points(const point_t *p, const point_t *q) {
  line_t r;
  r.a = p->y - q->y;
  r.b = q->x - p->x;
  r.c = -r.a*p->x - r.b*p->y;
  line_norm(&r);
  return r;
}
///////////////////////////////////////////////////////

line_t line_from_coords(double x0, double y0, double x1, double y1) {
  point_t a = {.x = x0, .y = y0};
  point_t b = {.x = x1, .y = y1};
  return line_from_points(&a, &b);
}
///////////////////////////////////////////////////////

void line_norm(line_t *l) {
  double z = sqrt (l->a*l->a + l->b*l->b);
  if (fabs(z) <= EPS)
    return;
  l->a /= z;
  l->b /= z;
  l->c /= z;
}
///////////////////////////////////////////////////////
///////////////////////////////////////////////////////

static inline double det(double a, double b, double c, double d) {
  return a*d - b*c;
}
///////////////////////////////////////////////////////

liec_t line_intersection(const line_t *l, const line_t *r, point_t *res) {
  double d1 = det(l->a, l->b, r->a, r->b);
  double d2 = det(l->a, l->c, r->a, r->c);
  double d3 = det(l->b, l->c, r->b, r->c);
  double d4 = det(l->c, l->b, r->c, r->b);
  if (fabs(d1) <= EPS) {
    return (fabs(d2) <= EPS) && (fabs(d3) <= EPS) ? LIEC_INFINITY : LIEC_PARALLEL;
  }

  res->x = -d4 / d1;
  res->y = -d2 / d1;
  return LIEC_ONE_POINT;
}
///////////////////////////////////////////////////////

void test_line_perpendicular()
{
  line_t l1 = line_from_coords(-1.0, -1.0, 1.0, 1.0);
  line_t l2 = line_from_coords(-1.0, 1.0, 1.0, -1.0);
  point_t p1 = {.x = 0.0, .y = 0.0};
  point_t p2 = {.x = 1.0, .y = 0.0};
  line_t lp1 = line_perpendicular(&l1, &p1);
  line_t lp2 = line_perpendicular(&l1, &p2);

  printf("l1 -> a=%fl, b=%fl, c=%fl\n", l1.a, l1.b, l1.c);
  printf("l2 -> a=%fl, b=%fl, c=%fl\n", l2.a, l2.b, l2.c);
  printf("lp1 -> a=%fl, b=%fl, c=%fl\n", lp1.a, lp1.b, lp1.c);
  printf("lp2 -> a=%fl, b=%fl, c=%fl\n", lp2.a, lp2.b, lp2.c);

  liec_t li1 = line_intersection(&l2, &lp1, &p1);
  assert(li1 == LIEC_INFINITY);
  liec_t li2 = line_intersection(&lp1, &lp2, &p1);
  assert(li2 == LIEC_PARALLEL);
}

typedef struct test_case{
  line_t l1;
  line_t l2;
  point_t pi;
  liec_t ri;
  char *name;
} tc_t ;

void test_line_intersections()
{
  tc_t test_cases[] = {
    {
      .l1 = line_from_coords(0.0, 0.0, 1.0, 1.0),
      .l2 = line_from_coords(1.0, 0.0, 0.0, 1.0),
      .ri = LIEC_ONE_POINT,
      .name = "test1",
    },
    {
      .l1 = line_from_coords(1.0, 0.0, 1.0, 1.0),
      .l2 = line_from_coords(0.0, 0.0, 0.0, 1.0),
      .ri = LIEC_PARALLEL,
      .name = "test2",
    },
    {
      .name = NULL,
    },
  };

  for (tc_t *tc=test_cases; tc->name; ++tc) {
    liec_t act = line_intersection(&tc->l1, &tc->l2, &tc->pi);
    if (act != tc->ri) {
      printf("expected: %d, actual: %d\ntest %s fail", tc->ri, act, tc->name);
      continue;
    }

    printf("act == exp %d\n", tc->ri);
    if (act == LIEC_INFINITY) {
      printf("infinitive\n");
      continue;
    } else if (act == LIEC_PARALLEL) {
      printf("parallel\n");
      continue;
    }

    printf("intersection{x=%06fl, y=%06fl}\n", tc->pi.x, tc->pi.y);
  }
}
///////////////////////////////////////////////////////

