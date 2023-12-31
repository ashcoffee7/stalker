#include "scene.h"
#include "body.h"
#include "forces.h"
#include "list.h"
#include "sdl_wrapper.h"
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

const size_t REASONABLE_GUESS = 30;

typedef struct scene {
  list_t *bodies;
  list_t *forces;
} scene_t;

typedef void (*force_creator_t)(void *aux);

void void_body_free2(void *p) { body_free(p); }

scene_t *scene_init(void) {
  scene_t *scene = malloc(sizeof(scene_t));
  assert(scene != NULL);
  scene->bodies = list_init(REASONABLE_GUESS, (free_func_t)body_free);
  scene->forces = list_init(REASONABLE_GUESS, (free_func_t)force_free);
  return scene;
}

void scene_free(scene_t *scene) {
  list_free(scene->bodies);
  list_free(scene->forces);

  free(scene);
}

size_t scene_bodies(scene_t *scene) { return list_size(scene->bodies); }

body_t *scene_get_body(scene_t *scene, size_t index) {
  return list_get(scene->bodies, index);
}

list_t *scene_get_bodies(scene_t *scene) { return scene->bodies; }

void scene_add_body(scene_t *scene, body_t *body) {
  list_add(scene->bodies, body);
}

void scene_remove_body(scene_t *scene, size_t index) {
  assert(index < scene_bodies(scene));
  body_remove(scene_get_body(scene, index));
}

void scene_add_bodies_force_creator(scene_t *scene, force_creator_t forcer,
                                    void *aux, list_t *bodies,
                                    free_func_t freer) {
  force_t *force = force_init_with_bodies(aux, forcer, bodies, freer);
  list_add(scene->forces, force);
}

void scene_add_force_creator(scene_t *scene, force_creator_t forcer, void *aux,
                             free_func_t freer) {
  list_t *bodies = list_init(REASONABLE_GUESS, NULL);
  scene_add_bodies_force_creator(scene, forcer, aux, bodies, freer);
}

void scene_tick(scene_t *scene, double dt) {
  list_t *forces_list = scene->forces;

  for (size_t d = 0; d < list_size(forces_list); d++) {
    force_t *curr_force = (force_t *)list_get(forces_list, d);
    curr_force->forcer(curr_force->aux);
  }

  for (size_t i = 0; i < scene_bodies(scene); i++) {
    body_t *curr_body = scene_get_body(scene, i);
    body_tick(curr_body, dt);
  }

  for (size_t f = 0; f < list_size(forces_list); f++) {
    force_t *curr_force = (force_t *)list_get(forces_list, f);
    list_t *bodies = curr_force->bodies;
    for (size_t b = 0; b < list_size(bodies); b++) {
      body_t *curr_body = list_get(bodies, b);
      if (body_is_removed(curr_body) == true) {
        list_remove(forces_list, f);
        force_free(curr_force);
        f--;
        break;
      }
    }
  }
  list_t *bodies_scene = scene_get_bodies(scene);
  for (size_t b = 0; b < scene_bodies(scene); b++) {
    body_t *curr = scene_get_body(scene, b);
    if (body_is_removed(curr)) {
      list_remove(bodies_scene, b);
      body_free(curr);
      b--;
    }
  }
}

