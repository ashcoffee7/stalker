#include "forces.h"
#include "body.h"
#include "collision.h"
#include "list.h"
#include "math.h"
#include "scene.h"
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

void aux_freeinator(void *aux) { free(aux); }

typedef struct collision_aux {
  bool already_collided;
  vector_t axis;
  collision_handler_t handler;
  void *aux;
  body_t *body1;
  body_t *body2;
} collision_aux_t;

typedef struct impulse_aux {
  double elasticity;
  body_t *body1;
  body_t *body2;
} impulse_aux_t;

impulse_aux_t *impulse_aux_init(double elasticity, body_t *body1,
                                body_t *body2) {
  impulse_aux_t *impulse = malloc(sizeof(impulse_aux_t));
  impulse->elasticity = elasticity;
  impulse->body1 = body1;
  impulse->body2 = body2;
  return impulse;
}

collision_aux_t *collision_aux_init(void *aux, body_t *body1, body_t *body2,
                                    collision_handler_t handler) {
  collision_aux_t *out = malloc(sizeof(collision_aux_t));
  out->aux = aux;
  out->already_collided = false;
  out->handler = handler;
  out->body1 = body1;
  out->body2 = body2;
  return out;
}

void collision_free(collision_aux_t *collision_aux) {
  free(collision_aux->aux);
  free(collision_aux);
}

void force_free(force_t *force) {
  if (force->freer != NULL) {
    force->freer(force->aux);
  }
  while (list_size(force->bodies) > 0) {
    list_remove(force->bodies, 0);
  }
  if (force->bodies != NULL) {
    list_free(force->bodies);
  }
  free(force);
}

force_t *force_init(void *aux, force_creator_t forcer, free_func_t freer) {
  force_t *force = malloc(sizeof(force_t));
  assert(force != NULL);
  force->aux = aux;
  force->forcer = forcer;
  force->freer = freer;
  force->bodies = NULL;
  return force;
}

force_t *force_init_with_bodies(void *aux, force_creator_t forcer,
                                list_t *bodies, free_func_t freer) {
  force_t *force = malloc(sizeof(force_t));
  assert(force != NULL);
  force->aux = aux;
  force->forcer = forcer;
  force->bodies = bodies;
  force->freer = freer;
  return force;
}

typedef struct two_body_aux {
  double constant;
  body_t *body1;
  body_t *body2;
} two_body_aux_t;

two_body_aux_t *two_body_init(double constant, body_t *body1, body_t *body2) {
  two_body_aux_t *out = malloc(sizeof(two_body_aux_t));
  assert(out != NULL);
  out->constant = constant;
  out->body1 = body1;
  out->body2 = body2;
  return out;
}

void create_newtonian_gravity(scene_t *scene, double G, body_t *body1,
                              body_t *body2) {
  two_body_aux_t *gravity_aux = two_body_init(G, body1, body2);
  list_t *bodies = list_init(2, NULL);
  list_add(bodies, body1);
  list_add(bodies, body2);
  scene_add_bodies_force_creator(scene,
                                 (force_creator_t)apply_newtonian_gravity,
                                 gravity_aux, bodies, aux_freeinator);
}

void apply_newtonian_gravity(void *aux) {
  two_body_aux_t *grav_aux = (two_body_aux_t *)aux;
  body_t *body1 = grav_aux->body1;
  body_t *body2 = grav_aux->body2;
  double body1_mass = body_get_mass(body1);
  double body2_mass = body_get_mass(body2);
  vector_t centroid1 = body_get_centroid(body1);
  vector_t centroid2 = body_get_centroid(body2);
  double distance = sqrt(vec_dot(vec_subtract(centroid1, centroid2),
                                 vec_subtract(centroid1, centroid2)));
  if (distance > 5) {
    double force_numerator =
        ((grav_aux->constant) * (body1_mass) * (body2_mass));
    double grav = force_numerator / (distance * distance) / distance;
    vector_t grav_force2 =
        vec_multiply(grav, vec_subtract(centroid1, centroid2));
    vector_t grav_force1 = vec_negate(grav_force2);
    body_add_force(grav_aux->body1, grav_force1);
    body_add_force(grav_aux->body2, grav_force2);
  }
}

void create_spring(scene_t *scene, double k, body_t *body1, body_t *body2) {
  two_body_aux_t *spring_aux = two_body_init(k, body1, body2);
  list_t *bodies = list_init(2, NULL);
  list_add(bodies, body1);
  list_add(bodies, body2);
  scene_add_bodies_force_creator(scene, (force_creator_t)apply_spring_force,
                                 spring_aux, bodies, aux_freeinator);
}

void apply_spring_force(void *aux) {
  two_body_aux_t *spring_aux = (two_body_aux_t *)aux;
  body_t *body1 = spring_aux->body1;
  body_t *body2 = spring_aux->body2;
  vector_t body1_centroid = body_get_centroid(body1);
  vector_t body2_centroid = body_get_centroid(body2);
  vector_t spring_force1 = vec_multiply(
      spring_aux->constant, vec_subtract(body2_centroid, body1_centroid));
  vector_t spring_force2 = vec_negate(spring_force1);
  body_add_force(spring_aux->body1, spring_force1);
  body_add_force(spring_aux->body2, spring_force2);
}

void apply_vortex(void *aux) {
  two_body_aux_t *grav_aux = (two_body_aux_t *)aux;
  body_t *body1 = grav_aux->body1;
  body_t *body2 = grav_aux->body2;
  double body1_mass = body_get_mass(body1);
  double body2_mass = body_get_mass(body2);
  vector_t centroid1 = body_get_centroid(body1);
  vector_t centroid2 = body_get_centroid(body2);
  double distance = sqrt(vec_dot(vec_subtract(centroid1, centroid2),
                                 vec_subtract(centroid1, centroid2)));
  if (distance > 5) {
    double force_numerator =
        ((grav_aux->constant) * (body1_mass) * (body2_mass));
    double grav = force_numerator / (distance * distance) / distance;
    vector_t grav_force2 =
        vec_multiply(grav, vec_subtract(centroid1, centroid2));
    vector_t grav_force1 = vec_negate(grav_force2);
    body_add_force(grav_aux->body1, grav_force1);
    body_add_force(grav_aux->body2, grav_force2);
  }
}

void create_vortex(scene_t *scene, double G, body_t *body1,
                              body_t *body2) {
  two_body_aux_t *gravity_aux = two_body_init(G, body1, body2);
  list_t *bodies = list_init(2, NULL);
  list_add(bodies, body1);
  list_add(bodies, body2);
  scene_add_bodies_force_creator(scene,
                                 (force_creator_t)apply_vortex,
                                 gravity_aux, bodies, aux_freeinator);
}

typedef struct one_body_aux {
  double gamma;
  body_t *body;
} one_body_aux_t;

one_body_aux_t *one_body_init(double gamma, body_t *body) {
  one_body_aux_t *out = malloc(sizeof(one_body_aux_t));
  assert(out != NULL);
  out->gamma = gamma;
  out->body = body;
  return out;
}

void create_drag(scene_t *scene, double gamma, body_t *body) {
  one_body_aux_t *one_body_aux = one_body_init(gamma, body);
  list_t *bodies = list_init(2, NULL);
  list_add(bodies, body);
  scene_add_bodies_force_creator(scene, (force_creator_t)apply_drag_force,
                                 one_body_aux, bodies, aux_freeinator);
}

void apply_drag_force(void *aux) {
  one_body_aux_t *one_body_aux = (one_body_aux_t *)aux;
  body_t *curr_body = one_body_aux->body;
  vector_t curr_bod_vel = body_get_velocity(curr_body);
  vector_t drag_force =
      vec_negate(vec_multiply(one_body_aux->gamma, curr_bod_vel));
  body_add_force(one_body_aux->body, drag_force);
}

void apply_destructive_collision(body_t *body1, body_t *body2, vector_t axis, void *aux) {
  body_remove(body1);
  body_remove(body2);
}

void create_destructive_collision(scene_t *scene, body_t *body1,
                                  body_t *body2) {
  impulse_aux_t *impulse = impulse_aux_init(0, body1, body2);
  create_collision(scene, body1, body2,
                   (collision_handler_t)apply_destructive_collision, impulse,
                   (free_func_t)collision_free);
}

void apply_physics_collision(body_t *body1, body_t *body2, vector_t axis, void *aux) {
  impulse_aux_t *bounce_aux = (impulse_aux_t *)aux;
  double elasticity = bounce_aux->elasticity;
  double mass1 = body_get_mass(body1);
  double mass2 = body_get_mass(body2);
  double imp_const = ((mass1 * mass2) / (mass1 + mass2));
  if (mass1 == INFINITY) {
    imp_const = mass2;
  }
  if (mass2 == INFINITY) {
    imp_const = mass1;
  }
  double impulse_mag = vec_dot(body_get_velocity(body2), axis) -
                       (vec_dot(body_get_velocity(body1), axis));
  vector_t impulse = vec_multiply(impulse_mag, axis);
  double impulse_num = imp_const * (1 + elasticity);
  vector_t added_impulse = vec_multiply(impulse_num, impulse);
  body_add_impulse(body2, vec_negate(added_impulse));
  body_add_impulse(body1, added_impulse);
}

void create_physics_collision(scene_t *scene, double elasticity, body_t *body1,
                              body_t *body2) {
  impulse_aux_t *impulse = impulse_aux_init(elasticity, body1, body2);
  create_collision(scene, body1, body2,
                   (collision_handler_t)apply_physics_collision, impulse,
                   (free_func_t)collision_free);
}

void apply_delete_bounce(body_t *body1, body_t *body2, vector_t axis, void *aux) {
  apply_physics_collision(body1, body2, axis, aux);
  body_remove(body1);
}

void create_delete_bounce(scene_t *scene, double elasticity, body_t *delete,
                          body_t *bounce) {
  impulse_aux_t *impulse = impulse_aux_init(elasticity, delete, bounce);
  create_collision(scene, delete, bounce,
                   (collision_handler_t)apply_delete_bounce, impulse,
                   (free_func_t)collision_free);
}

void apply_coin_collecting(body_t *body1, body_t *body2, vector_t axis, void *aux) {
  body_set_coins(body1);
  body_remove(body2);
}

void create_coin_collecting(scene_t *scene, body_t *player, body_t *coin) {
  impulse_aux_t *impulse = impulse_aux_init(0, player, coin);
  create_collision(scene, player, coin, (collision_handler_t) apply_coin_collecting, impulse, (free_func_t) collision_free);
}

void apply_collision_velocity(body_t *body1, body_t *body2, vector_t axis, void *aux) {
  body_set_velocity(body1, vec_multiply(0.25, body_get_velocity(body1)));
  body_remove(body2);
}

void create_collision_velocity(scene_t *scene, body_t *body1, body_t *body2) {
  impulse_aux_t *aux = impulse_aux_init(0, body1, body2);
  create_collision(scene, body1, body2, (collision_handler_t) apply_collision_velocity, aux, (free_func_t) collision_free);

}

void apply_end_game(body_t *body1, body_t *body2, vector_t axis, void *aux) {
  body_remove(body1);
  body_remove(body2);
}

void create_end_game(scene_t *scene, body_t *body1, body_t *body2) {
  impulse_aux_t *aux = impulse_aux_init(0, body1, body2);
  create_collision(scene, body1, body2, (collision_handler_t) apply_end_game, aux, (free_func_t) collision_free);
}

void create_collision(scene_t *scene, body_t *body1, body_t *body2,
                      collision_handler_t collider, void *aux,
                      free_func_t freer) {
  impulse_aux_t *impulse = (impulse_aux_t *)aux;
  collision_aux_t *collider_aux =
      collision_aux_init(impulse, body1, body2, collider);
  list_t *bodies = list_init(2, NULL);
  collider_aux->already_collided = false;
  list_add(bodies, body1);
  list_add(bodies, body2);
  scene_add_bodies_force_creator(scene, (force_creator_t)apply_collision,
                                 collider_aux, bodies, freer);
}

void apply_collision(void *aux) {
  collision_aux_t *collider_aux = (collision_aux_t *)aux;
  vector_t axis = collision_vec(body_get_shape(collider_aux->body1),
                          body_get_shape(collider_aux->body2));
  if (collider_aux->already_collided == false) {
    if (collision_checker(body_get_shape(collider_aux->body1),
                          body_get_shape(collider_aux->body2)) == true) {
          (collider_aux->handler)(collider_aux->body1, collider_aux->body2, axis, collider_aux->aux);
    }
  }
  collider_aux->already_collided = collision_checker(
      body_get_shape(collider_aux->body1), body_get_shape(collider_aux->body2));
}