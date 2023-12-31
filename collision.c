#include "collision.h"
#include "list.h"
#include "vector.h"
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

bool collision_checker(list_t *shape1, list_t *shape2) {
  collision_info_t *collision = find_collision(shape1, shape2);
  bool truth = collision->collided;
  free(collision);
  return truth;
}

vector_t collision_vec(list_t *shape1, list_t *shape2) {
  collision_info_t *collision = find_collision(shape1, shape2);
  vector_t ax = collision->axis;
  free(collision);
  return ax;
}

/// @brief
/// @param shape1
/// @param shape2
/// @return
collision_info_t *find_collision(list_t *shape1, list_t *shape2) {
  collision_info_t *collision = malloc(sizeof(collision_info_t));
  float minimum_overlap = INFINITY;
  vector_t reflecting_axis;

  size_t num_vertices_shape1 = list_size(shape1);
  size_t num_vertices_shape2 = list_size(shape2);

  for (size_t i = 0; i < num_vertices_shape1 + num_vertices_shape2; i++) {
    vector_t *normal = malloc(sizeof(vector_t));

    // normal vector for edges of shape1
    if (i < num_vertices_shape1) {
      vector_t *point_a = (vector_t *)list_get(shape1, i);
      vector_t *point_b =
          (vector_t *)list_get(shape1, (i + 1) % num_vertices_shape1);

      normal->x = point_b->y - point_a->y;
      normal->y = -(point_b->x - point_a->x);
    }

    // normal vector for edges of shape2
    else {
      vector_t *point_a = (vector_t *)list_get(shape2, i - num_vertices_shape1);
      vector_t *point_b = (vector_t *)list_get(
          shape2, (i - num_vertices_shape1 + 1) % num_vertices_shape2);

      normal->x = point_b->y - point_a->y;
      normal->y = -(point_b->x - point_a->x);
    }

    float min_1 = INFINITY;
    float min_2 = INFINITY;

    float max_1 = -INFINITY;
    float max_2 = -INFINITY;

    // projection of each vertex onto axis (normal vector)
    for (size_t j = 0; j < num_vertices_shape1; j++) {
      vector_t *vertex = (vector_t *)list_get(shape1, j);
      float proj_shape1 = vertex->x * normal->x + vertex->y * normal->y;

      if (proj_shape1 < min_1) {
        min_1 = proj_shape1;
      }
      if (proj_shape1 > max_1) {
        max_1 = proj_shape1;
      }
    }

    for (size_t j = 0; j < num_vertices_shape2; j++) {
      vector_t *vertex = (vector_t *)list_get(shape2, j);
      float proj_shape2 = vertex->x * normal->x + vertex->y * normal->y;

      if (proj_shape2 < min_2) {
        min_2 = proj_shape2;
      }
      if (proj_shape2 > max_2) {
        max_2 = proj_shape2;
      }
    }

    // all axes must overlap for collision to be true
    if (max_1 < min_2 || max_2 < min_1) {
      // No overlap, i.e polygons do not intersect
      collision->collided = false;
      free(normal);
      list_free(shape1);
      list_free(shape2);
      return collision;
    }

    else {
      // set reflecting vector to axis of minimal overlap
      float x_squared = (normal->x) * (normal->x);
      float y_squared = (normal->y) * (normal->y);
      float magnitude = sqrt(x_squared + y_squared);

      if (max_1 >= max_2) {
        if (max_2 - min_1 < minimum_overlap) {
          reflecting_axis.x =(normal->x) / magnitude;
          reflecting_axis.y = (normal->y) / magnitude;
          minimum_overlap = max_2 - min_1;
        }
      }

      else {
        if (max_1 - min_2 < minimum_overlap) {
          reflecting_axis.x = normal->x / magnitude;
          reflecting_axis.y = normal->y / magnitude;
          minimum_overlap = max_1 - min_2;
        }
      }
    }
    free(normal);
  }

  // all axes overlap
  collision->axis = reflecting_axis;
  collision->collided = true;
  list_free(shape1);
  list_free(shape2);

  return collision;
}