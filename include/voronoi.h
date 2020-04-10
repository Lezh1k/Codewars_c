#ifndef VORONOI_H
#define VORONOI_H

typedef struct point {
  double x, y;
} point_t;

typedef struct line {
  double a, b, c; //Ax + By + C = 0
} line_t;

typedef struct vec {
  point_t a, b;
} vec_t;

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


line_t line_perpendicular(const line_t *l,
                          const point_t *p);

// will return val == -1 if lines are parallel, 1 if lines are equal and 0 if there is intersection between them
liec_t line_intersection(const line_t *l,
                         const line_t *r,
                         point_t *res);

///////////////////////////////////////////////////////
///////////////////////////////////////////////////////

void test_line_intersections(void);
void test_line_perpendicular(void);


#endif // VORONOI_H
