#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "voronoi.h"

static void line_normalization(line_t *l);
static int cmp_point_by_polar_angle(const void *l, const void *r);
static bool is_figure_closed(point_t *edges, size_t N);
static double locus_area(const point_t *arr,
                         size_t N,
                         size_t dstIX);

static void voronoi_filter_edges(const point_t *siteCenter, point_t *edges,
                                 size_t *eN,
                                 const line_t *middle_normals,
                                 size_t mnN);


point_t point(double x, double y) {
  point_t r = {x, y};
  return r;
}
///////////////////////////////////////////////////////

point_t point_middle(const point_t *a,
                     const point_t *b) {
  return point((a->x + b->x) / 2.0, (a->y + b->y) / 2.0);
}
///////////////////////////////////////////////////////

line_t line_normal(const line_t *l,
                   const point_t *p) {
  // A(y-y1) - B(x-x1) = 0
  line_t r;
  r.a = -l->b;
  r.b = l->a;
  r.c = l->b*p->x - l->a*p->y;
  return r;
}
///////////////////////////////////////////////////////

line_t line_from_points(const point_t *p, const point_t *q) {
  return line_from_coords(p->x, p->y, q->x, q->y);
}
///////////////////////////////////////////////////////

line_t line_from_coords(double x0, double y0,
                        double x1, double y1) {
  line_t r;
  r.a = y1 - y0;
  r.b = x0 - x1;
  r.c = -r.a*x0 - r.b*y0;
  line_normalization(&r);
  return r;
}
///////////////////////////////////////////////////////

void line_normalization(line_t *l) {
  double z = sqrt (l->a*l->a + l->b*l->b);
  if (fabs(z) < VORONOI_EPS)
    return;
  l->a /= z;
  l->b /= z;
  l->c /= z;
}
///////////////////////////////////////////////////////
///////////////////////////////////////////////////////

static inline double det(double a,
                         double b,
                         double c,
                         double d) {
  return a*d - b*c;
}
///////////////////////////////////////////////////////

liec_t line_intersection(const line_t *l,
                         const line_t *r,
                         point_t *res) {
  //if perpendiculars are collinear then lines are collinear.
  //so line vector direction = (-b,a) and perpendicular(a,b)
  double zn = det(l->a, l->b, r->a, r->b);
  if (fabs(zn) < VORONOI_EPS)
    return LIEC_PARALLEL;
  double d2 = det(l->a, l->c, r->a, r->c);
  double d4 = det(l->c, l->b, r->c, r->b);
  res->x = -d4 / zn;
  res->y = -d2 / zn;
  return LIEC_ONE_POINT;
}
///////////////////////////////////////////////////////

double triangle_area(const point_t *p1,
                     const point_t *p2,
                     const point_t *p3) {
  double S2 = det(p2->x - p1->x, p2->y-p1->y, p3->x - p1->x, p3->y - p1->y);
  return S2 / 2.0;
}

///////////////////////////////////////////////////////

bool
points_are_on_same_side(const point_t *p1,
                        const point_t *p2,
                        const line_t *l) {
  double r1 = l->a*p1->x + l->b*p1->y + l->c;
  double r2 = l->a*p2->x + l->b*p2->y + l->c;
  if (fabs(r1) < VORONOI_EPS || fabs(r2) < VORONOI_EPS)
    return true;
  return r1*r2 >= 0.0; //?
}
///////////////////////////////////////////////////////

bool
is_figure_closed(point_t *edges, size_t N) {
  double angle = 0.0;
  double dt;
  for (size_t i = 0; i < N-1; ++i) {
    dt = edges[i].x*edges[i].y +
         edges[i+1].x*edges[i+1].y;
    angle += dt;
  }
  dt = edges[N-1].x*edges[N-1].y -
       edges[0].x*edges[0].y;
  angle += dt;
  return fabs(angle - 2.0) <= VORONOI_EPS;
}
///////////////////////////////////////////////////////

//be careful here. our center point is 0.0, 0.0
int
cmp_point_by_polar_angle(const void *l,
                         const void *r) {
  const point_t *p1 = (const point_t*)l;
  const point_t *p2 = (const point_t*)r;
  //  (x1-x0)*(y2-y0) - (x2-x0)*(y1-y0);
  double angle = p1->x*p2->y - p2->x*p1->y;
  if (fabs(angle) < VORONOI_EPS)
    return 0;
  if (angle < 0.0)
    return 1;
  return -1;
}
///////////////////////////////////////////////////////

void
voronoi_filter_edges(const point_t *siteCenter,
                     point_t *edges,
                     size_t *eN,
                     const line_t *middle_normals,
                     size_t mnN) {
  for (size_t mni = 0; mni < mnN; ++mni) {
    size_t k = 0;
    for (size_t ei = 0; ei < *eN; ++ei) {
      if (!points_are_on_same_side(siteCenter, &edges[ei], &middle_normals[mni]))
        continue;
      edges[k++] = edges[ei];
    }
    *eN = k;
  }
}
///////////////////////////////////////////////////////

double
locus_area(const point_t *arr,
           size_t N,
           size_t dstIX) {
  line_t *middle_normals = malloc((N-1) * sizeof (line_t));
  point_t *locus_edges = malloc((N-1)*(N-1) * sizeof(point_t)); //worst case O((n-1)^2)
  size_t edges_count = 0;
  line_t *pmn = middle_normals;

  for (size_t i = 0; i < dstIX; ++i) {
    point_t mp = point_middle(&arr[dstIX], &arr[i]);
    line_t l = line_from_points(&arr[dstIX], &arr[i]);
    *pmn++ = line_normal(&l, &mp);
  }

  for (size_t i = dstIX+1; i < N; ++i) {
    point_t mp = point_middle(&arr[dstIX], &arr[i]);
    line_t l = line_from_points(&arr[dstIX], &arr[i]);
    *pmn++ = line_normal(&l, &mp);
  }

  for (size_t i = 0; i < N-1; ++i) {
    for (size_t j = i+1; j < N-1; ++j) {
      liec_t liec = line_intersection(&middle_normals[i],
                                      &middle_normals[j],
                                      &locus_edges[edges_count]);
      if (liec != LIEC_ONE_POINT)
        continue; //what should we do if they are same? nothing I guess.
      ++edges_count;
    }
  }

  printf("\n before %ld", edges_count);
  double da, area = 0.0;
  point_t dst = arr[dstIX];
  do {
    voronoi_filter_edges(&dst, locus_edges, &edges_count, middle_normals, N-1);
    printf("\n after %ld", edges_count);
    if (!edges_count)
      break;

    //this is for our cmp point by polar angle function.
    //we move all points to the beginning of coordinates system
    for (size_t i = 0; i < edges_count; ++i) {
      locus_edges[i].x -= dst.x;
      locus_edges[i].y -= dst.y;
    }
    dst.x = dst.y = 0.0;

    qsort((void*)locus_edges, edges_count, sizeof(point_t), cmp_point_by_polar_angle);

    //check somehow that this points creates closed figure
    for (size_t i = 0; i < edges_count-1; ++i) {
      da = triangle_area(&dst, &locus_edges[i], &locus_edges[i+1]);
      area += da;
    }
    da = triangle_area(&dst, &locus_edges[edges_count-1], &locus_edges[0]);
    if (da < -VORONOI_EPS) {
      area = 0.0;
      break;
    }

    area += da;

  } while (0);

  free(middle_normals);
  free(locus_edges);

  if (fabs(area) <= VORONOI_EPS)
    area = -1.0;

  printf("\n%ld -> %f\n", dstIX, area);
  return area;
}
///////////////////////////////////////////////////////

void voronoi_areas(point_t p[], unsigned n, double areas[]) {
  printf("***points count: %d***", n);
  for (size_t i = 0; i < (size_t)n; ++i) {
    printf("{%f, %f},", p[i].x, p[i].y);
    areas[i] = locus_area(p, n, i);
  }
  printf("\n");
}
///////////////////////////////////////////////////////
