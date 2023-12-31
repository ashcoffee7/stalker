#include "vector.h"
#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdlib.h>

const vector_t VEC_ZERO = {0, 0};

vector_t vec_add(vector_t v1, vector_t v2) {
  vector_t out;
  out.x = v1.x + v2.x;
  out.y = v1.y + v2.y;
  return out;
}

vector_t vec_subtract(vector_t v1, vector_t v2) {
  vector_t out = vec_add(v1, vec_negate(v2));
  return out;
}

vector_t vec_negate(vector_t v) {
  vector_t out = vec_multiply(-1, v);
  return out;
}

vector_t vec_multiply(double scalar, vector_t v) {
  vector_t out;
  out.x = scalar * v.x;
  out.y = scalar * v.y;
  return out;
}

double vec_dot(vector_t v1, vector_t v2) {
  double out = v1.x * v2.x + v1.y * v2.y;
  return out;
}

double vec_cross(vector_t v1, vector_t v2) {
  double out = v1.x * v2.y - v1.y * v2.x;
  return out;
}

vector_t vec_rotate(vector_t v, double angle) {
  vector_t out;
  out.x = v.x * cos(angle) - v.y * sin(angle);
  out.y = v.x * sin(angle) + v.y * cos(angle);
  return out;
}