#ifndef VORONOI_H
#define VORONOI_H

#include <stdint.h>
#include <stdbool.h>
#define VORONOI_EPS 1.0e-7

typedef struct point {
  double x, y;
} point_t;

point_t point(double x, double y);
point_t point_middle(const point_t *a, const point_t *b);

typedef struct line {
  double a, b, c; //Ax + By + C = 0
} line_t;

typedef enum line_intersection_edge_cases {
  LIEC_PARALLEL = -1,
  LIEC_ONE_POINT = 0,
  LIEC_INFINITY = 1
} liec_t;

line_t line_from_points(const point_t *p,
                        const point_t *q);

line_t line_from_coords(double x0,
                        double y0,
                        double x1,
                        double y1);


line_t line_normal(const line_t *l,
                   const point_t *p);

// will return val == -1 if lines are parallel, 1 if lines are equal and 0 if there is intersection between them
liec_t line_intersection(const line_t *l,
                         const line_t *r,
                         point_t *res);

bool points_are_on_same_side(const point_t *p1,
                             const point_t *p2,
                             const line_t *l);

double triangle_area(const point_t *p1,
                     const point_t *p2,
                     const point_t *p3);

void voronoi_areas(point_t p[], unsigned n, double areas[]);

///////////////////////////////////////////////////////
///////////////////////////////////////////////////////
#endif // VORONOI_H
