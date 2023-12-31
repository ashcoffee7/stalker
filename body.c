#include "body.h"
#include "assert.h"
#include "list.h"
#include "polygon.h"
#include <stdlib.h>

typedef struct body {
  list_t *shape;
  double mass;
  size_t coins;
  rgb_color_t color;
  vector_t centroid;
  vector_t velocity;
  double angle;
  vector_t f;
  vector_t i;
  bool status;
  void *info;
  free_func_t info_freer;
  char *texture_link;
} body_t;

void info_freer(void *info) { free(info); }

body_t *body_init(list_t *shape, double mass, rgb_color_t color, char* link) {
  body_t *out = malloc(sizeof(body_t));
  assert(out != NULL);
  out->shape = shape;
  out->mass = mass;
  out->coins = 0;
  out->color = color;
  out->velocity = (vector_t){.x = 0, .y = 0};
  out->angle = 0;
  out->centroid = polygon_centroid(shape);
  out->f = (vector_t){.x = 0, .y = 0};
  out->i = (vector_t){.x = 0, .y = 0};
  out->status = false;
  out->info = NULL;
  out->info_freer = NULL;
  out->texture_link = link;
  return out;
}

body_t *body_init_with_info(list_t *shape, double mass, rgb_color_t color,
                            void *info, free_func_t info_freer2, char* link) {
  body_t *out = malloc(sizeof(body_t));
  assert(out != NULL);
  out->shape = shape;
  out->mass = mass;
  out->coins = 0;
  out->color = color;
  out->centroid = polygon_centroid(shape);
  out->velocity = (vector_t){.x = 0, .y = 0};
  out->angle = 0;
  out->f = (vector_t){.x = 0, .y = 0};
  out->i = (vector_t){.x = 0, .y = 0};
  out->status = false;
  out->info = info;
  out->info_freer = info_freer2;
  out->texture_link = link;
  return out;
}

void body_free(body_t *body) {
  list_free(body->shape);
  if (body->info_freer != NULL && body->info != NULL) {
    body->info_freer(body->info);
  }
  free(body);
}

list_t *body_get_shape(body_t *body) {
  list_t *out = list_init(list_size(body->shape), (free_func_t)free);
  for (size_t i = 0; i < list_size(body->shape); i++) {
    vector_t *original = list_get(body->shape, i);
    vector_t *copy = malloc(sizeof(vector_t));
    copy->x = original->x;
    copy->y = original->y;
    list_add(out, copy);
  }
  return out;
}

vector_t body_get_centroid(body_t *body) { return body->centroid; }

size_t body_get_coins(body_t *body) { return body->coins; }

vector_t body_get_velocity(body_t *body) { return body->velocity; }

double body_get_mass(body_t *body) { return body->mass; }

rgb_color_t body_get_color(body_t *body) { return body->color; }

void body_set_color(body_t *body, rgb_color_t color) {
  body->color = color;
}

void *body_get_info(body_t *body) { return body->info; }

void body_set_centroid(body_t *body, vector_t x) {
  for (size_t i = 0; i < list_size(body->shape); i++) {
    vector_t *point = list_get(body->shape, i);
    *point = vec_add(x, vec_subtract(*point, body->centroid));
  }
  body->centroid = x;
}

char* body_get_texture(body_t *body) {
  return body->texture_link;
}

void body_set_texture(body_t *body, char *link) {
  body->texture_link = link;
}
void body_set_coins(body_t * body) {
  size_t new_coins = body->coins + 1;
  body->coins = new_coins;
}

void body_set_velocity(body_t *body, vector_t v) { body->velocity = v; }

void body_set_rotation(body_t *body, double angle) {
  polygon_rotate(body->shape, angle - body->angle, body->centroid);
  body->angle = angle;
}

void body_add_force(body_t *body, vector_t force) {
  vector_t forc = vec_add(body->f, force);
  body->f = forc;
}

void body_add_impulse(body_t *body, vector_t impulse) {
  vector_t imp = vec_add(body->i, impulse);
  body->i = imp;
}

void body_tick(body_t *body, double dt) {
  vector_t acceleration = vec_multiply((1 / body->mass), body->f);
  vector_t velocity_force = vec_multiply(dt, acceleration);
  vector_t velocity_impulse = vec_multiply((1 / body->mass), body->i);
  vector_t velocity_after = vec_add(velocity_force, velocity_impulse);
  vector_t new_velocity = vec_add(body->velocity, velocity_after);
  vector_t velocity_doubled = vec_add(body->velocity, new_velocity);
  vector_t velocity = vec_multiply(0.5, velocity_doubled);
  body_set_centroid(body, vec_add(body->centroid, vec_multiply(dt, velocity)));
  body->f = (vector_t){.x = 0, .y = 0};
  body->i = (vector_t){.x = 0, .y = 0};
  body_set_velocity(body, new_velocity);
}

void body_remove(body_t *body) {
  if (body->status != true) {
    body->status = true;
  }
}

bool body_is_removed(body_t *body) { return body->status; }
