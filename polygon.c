#include "gen_list.h"
#include "list.h"
#include "vector.h"

double polygon_area(list_t *polygon) {
  double area = 0;

  for (size_t i = 0; i < list_size(polygon) - 1; i++) {
    area += ((vector_t *)list_get(polygon, i))->x *
                ((vector_t *)list_get(polygon, i + 1))->y -
            ((vector_t *)list_get(polygon, i + 1))->x *
                ((vector_t *)list_get(polygon, i))->y;
  }

  area += ((vector_t *)list_get(polygon, list_size(polygon) - 1))->x *
              ((vector_t *)list_get(polygon, 0))->y -
          ((vector_t *)list_get(polygon, list_size(polygon) - 1))->y *
              ((vector_t *)list_get(polygon, 0))->x;

  area /= 2;
  return area;
}

vector_t polygon_centroid(list_t *polygon) {
  vector_t out;
  out.x = 0;
  out.y = 0;

  for (size_t i = 0; i < list_size(polygon) - 1; i++) {
    vector_t *v1 = list_get(polygon, i);
    vector_t *v2 = list_get(polygon, i + 1);
    out.x += (v1->x + v2->x) * vec_cross(*v1, *v2);
    out.y += (v1->y + v2->y) * vec_cross(*v1, *v2);
  }

  vector_t *v1 = list_get(polygon, list_size(polygon) - 1);
  vector_t *v2 = list_get(polygon, 0);
  out.x += (v1->x + v2->x) * vec_cross(*v1, *v2);
  out.y += (v1->y + v2->y) * vec_cross(*v1, *v2);
  double area = polygon_area(polygon);
  out.x /= (6 * area);
  out.y /= (6 * area);
  return out;
}

void polygon_translate(list_t *polygon, vector_t translation) {
  for (size_t i = 0; i < list_size(polygon); i++) {
    vector_t *v = list_get(polygon, i);
    *v = vec_add(*v, translation);
  }
}

void polygon_rotate(gen_list_t *polygon, double angle, vector_t point) {
  for (size_t i = 0; i < list_size((list_t *) polygon); i++) {
    vector_t *v = list_get((list_t *) polygon, i);
    *v = vec_add(point, vec_rotate(vec_subtract(*v, point), angle));
  }
}