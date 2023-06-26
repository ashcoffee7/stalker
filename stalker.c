#include "body.h"
#include "collision.h"
#include "color.h"
#include "forces.h"
#include "list.h"
#include "polygon.h"
#include "scene.h"
#include "sdl_wrapper.h"
#include "state.h"
#include "vector.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

// all constants
// CHARACTER INITIALIZATIONS
// test "player" constants
const double PLAYER_MASS = 600;
const double PLAYER_RADIUS = 20;
const double PLAYER_WIDTH = PLAYER_MASS * 2;
const double NUM_POINTS_PLAYER = 100;
const rgb_color_t PLAYER_COLOR = (rgb_color_t){.r = 1.0, .g = 0.5, .b = 1.0};
const vector_t WINDOW = (vector_t){.x = 1000, .y = 500};
const vector_t PLAYER_CENTER = (vector_t){.x = 500, .y = 250};
const double GRAVITY = 1500;
const double PELLET_CHANCE  = 0.002;
// stalker constants
const double STALKER_MASS = 200;
const double STALKER_RADIUS = 30;
const double NUM_POINTS_STALKER = 100;
const vector_t STALKER_CENTER = (vector_t){.x = 500, .y = 400};
const rgb_color_t STALKER_COLOR = (rgb_color_t){.r = 0.5, .g = 1.0, .b = 1.0};
const double STALKER_MIN_DIST = 100;
const double STALKER_MAX_DIST = 150;
// obstacle constants - physics collision (asteroids/debris,
// player bounces off these)
// for now, modeled as circles
const double EDGE_LIMIT_X = 100;
const double EDGE_LIMIT_Y = 50;
const vector_t EDGE_LIMIT_MIN = (vector_t){.x = 100, .y = 50};
// const vector_t EDGE_LIMIT_MAX = (vector_t) vec_subtract(WINDOW, EDGE_LIMIT_MIN);
const double OBSTACLE_MASS = 500;
const int OBSTACLE_MIN_RADIUS = 15;
const int OBSTACLE_MAX_RADIUS = 30;
const int MIN_DISTANCE_BETWEEN = 80;
const double NUM_POINTS_OBSTACLE = 100;
const rgb_color_t BOUNCING_OBSTACLE_COLOR = (rgb_color_t){.r = 1, .g = 0, .b = 0};
const double NUM_BOUNCING_OBSTACLES = 5;
const rgb_color_t REDUCEVEL_COLOR = (rgb_color_t){.r = 0, .g = 1, .b = 0};
const double NUM_REDUCEVEL_OBSTACLE = 5;
const rgb_color_t STALKERVEL_COLOR = (rgb_color_t){.r = 0, .g = 0, .b = 1.0};
const double NUM_STALKERVEL_OBSTACLE  = 2;
const double OBSTACLE_ELASTICITY = 1;

// coin constants
const rgb_color_t COIN_COLOR = (rgb_color_t){.r = 1.0, .g = 1.0, .b = 0.0};
const double COIN_MASS = 500;
const double COIN_RADIUS = 15;
const double NUM_POINTS_COIN = 100;
const double NUM_COINS = 0;
const double COINS_NEEDED = 0;

// time constants
const time_t TIMER = 30;

// wall constants
const double VERT_WALL_HEIGHT = 500 - (2 * 5.0);
const double VERT_WALL_WIDTH = 10.0;
const double HORIZ_WALL_HEIGHT = 10.0;
const double HORIZ_WALL_WIDTH = 1000;
const vector_t FIRST_CENTER = (vector_t){.x = 5, .y = 250};
const vector_t SECOND_CENTER = (vector_t){.x = 995, .y = 250};
const vector_t THIRD_CENTER = (vector_t){.x = 500, .y = 495};
const vector_t FOURTH_CENTER = (vector_t){.x = 500, .y = 5};
const rgb_color_t WALL_COLOR = (rgb_color_t){.r = 1.0, .g = 1.0, .b = 1.0};

// text
const size_t FONT_SIZE = 1;


// define state
typedef struct state {
  scene_t *scene;
  bool is_held;
  int last_dir_held;
  time_t start_time;
} state_t;

// define enum for teams
enum Team { ALLY_PLAYER, STALKER, OBSTACLE, COIN, ENEMY_WALL, BACKGROUND };

// define enum for sound effects
enum Sound { METEOR_HIT, SATELLITE_HIT, COIN_COLLECTED, VICTORY_SOUND, DEFEAT_SOUND };

// define info and init
typedef struct info {
  enum Team team;
} info_t;

// info_init
info_t *info_init(enum Team team) {
  info_t *out = malloc(sizeof(info_t));
  out->team = team;
  return out;
}

// make rectangle function for all bodies
body_t *make_rectangle(scene_t *scene, vector_t center, rgb_color_t color, double height,
                       double width, enum Team team, double mass, char* link) {
  info_t *info = info_init(team);
  list_t *shape = list_init(4, free);
  vector_t *v = malloc(sizeof(vector_t));
  v->x = center.x - (width / 2.0);
  v->y = center.y + (height / 2.0);
  list_add(shape, v);
  vector_t *a = malloc(sizeof(vector_t));
  a->x = center.x - (width / 2.0);
  a->y = center.y - (height / 2.0);
  list_add(shape, a);
  vector_t *c = malloc(sizeof(vector_t));
  c->x = center.x + (width / 2.0);
  c->y = center.y - (height / 2.0);
  list_add(shape, c);
  vector_t *b = malloc(sizeof(vector_t));
  b->x = center.x + (width / 2.0);
  b->y = center.y + (height / 2.0);
  list_add(shape, b);
  body_t *rectangle =
      body_init_with_info(shape, mass, color, info, (free_func_t)free, link);
  return rectangle;
}

// make background
void make_background(scene_t *scene, char* link) {
  vector_t center = (vector_t) {.x = 0, .y = 0};
  info_t *info = info_init(BACKGROUND);
  body_t *background = make_rectangle(scene, center, REDUCEVEL_COLOR, WINDOW.x - 1, WINDOW.y - 1, info->team, INFINITY, link);
  scene_add_body(scene, background);
}

void outcome(scene_t *scene, char* link) {
    make_background(scene, link);
}

// make_player (adds to the scene)
void make_player(scene_t *scene) {
  info_t *info = info_init(ALLY_PLAYER);
  body_t *player = make_rectangle(scene, PLAYER_CENTER, PLAYER_COLOR, 50.0, 40.0, info->team, PLAYER_MASS, "assets/player_down.png");
  scene_add_body(scene, player);
}


// keyboard controls
void on_key(char key, key_event_type_t type, double held_time, state_t *state) {
  scene_t *scene = state->scene;
  body_t *player = scene_get_body(scene, 1);

  if (type == KEY_PRESSED) {
    switch (key) {
    case A_KEY:
      state->is_held = 1;
      state->last_dir_held = 7;
      vector_t new_v1 = (vector_t){.x = -200, .y = 0};
      body_set_velocity(player, new_v1);
      break;

    case S_KEY:
      state->is_held = 1;
      state->last_dir_held = 6;
      vector_t new_v2 = (vector_t){.x = 0, .y = 200};
      body_set_velocity(player, new_v2);
      break;

    case W_KEY:
      state->is_held = 1;
      state->last_dir_held = 8;
      vector_t new_v3 = (vector_t){.x = 0, .y = -200};
      body_set_velocity(player, new_v3);
      break;

    case D_KEY:
      state->is_held = 1;
      state->last_dir_held = 9;
      vector_t new_v4 = (vector_t){.x = 200, .y = 0};
      body_set_velocity(player, new_v4);
      break;

    case SPACE_BAR:
      // jumping movement
      // more like a dash
      // printf("%i", state->last_dir_held);
      state->last_dir_held = state->last_dir_held;
      int last_dir = state->last_dir_held;
      state->is_held = 1;
      state->last_dir_held = 5;
      vector_t curr_vel = body_get_velocity(player);
      vector_t new_vel = vec_multiply(3, curr_vel);
      body_set_velocity(player, new_vel);
      // state->is_held = 0;
      state->last_dir_held = last_dir;
      // state->is_held = 1;
      break;
    }
  }
  if (type == KEY_RELEASED) {
    state->is_held = 0;
    vector_t zero_vector = (vector_t){.x = 0, .y = 0};
    body_set_velocity(player, zero_vector);
  }
}


void make_stalker(scene_t *scene) {
  info_t *info = info_init(STALKER);
  body_t *stalker = make_rectangle(scene, STALKER_CENTER, STALKER_COLOR, 40.0, 20.0, info->team, STALKER_MASS, "assets/stalker_up.png");
  body_t *player = scene_get_body(scene, 1);
  scene_add_body(scene, stalker);
  create_end_game(scene, player, stalker);
  create_newtonian_gravity(scene, GRAVITY, stalker, player);
  // whatever is being called first does not show up
}

// randomize obstacles and coins
vector_t randomize_center(scene_t *scene) {
  double x = (rand() % (size_t)(WINDOW.x + 100));
  double y = (rand() % (size_t)(WINDOW.y + 200));
  vector_t center = {.x = x, .y = y};
  vector_t player_center = body_get_centroid(scene_get_body(scene, 1));
  if ((center.x == player_center.x) && (center.y == player_center.y)) {
    randomize_center(scene);
  }
  return center;
}

void initialize_walls(scene_t *scene) {
  body_t *rectangle1 =
      make_rectangle(scene, FIRST_CENTER, WALL_COLOR, VERT_WALL_HEIGHT,
                     VERT_WALL_WIDTH, ENEMY_WALL, INFINITY, NULL);
  body_t *rectangle2 =
      make_rectangle(scene, SECOND_CENTER, WALL_COLOR, VERT_WALL_HEIGHT,
                     VERT_WALL_WIDTH, ENEMY_WALL, INFINITY, NULL);
  body_t *rectangle3 =
      make_rectangle(scene, THIRD_CENTER, WALL_COLOR, HORIZ_WALL_HEIGHT,
                     HORIZ_WALL_WIDTH, ENEMY_WALL, INFINITY, NULL);
  body_t *rectangle4 =
      make_rectangle(scene, FOURTH_CENTER, WALL_COLOR, HORIZ_WALL_HEIGHT,
                     HORIZ_WALL_WIDTH, ENEMY_WALL, INFINITY, NULL);
  scene_add_body(scene, rectangle1);
  scene_add_body(scene, rectangle2);
  scene_add_body(scene, rectangle3);
  scene_add_body(scene, rectangle4);
  body_t *stalker = scene_get_body(scene, 2);
  create_physics_collision(scene, 1, rectangle1, stalker);
  create_physics_collision(scene, 1, rectangle2, stalker);
  create_physics_collision(scene, 1, rectangle3, stalker);
  create_physics_collision(scene, 1, rectangle4, stalker);
}

void wrap_around(scene_t *scene, body_t *player) {
  vector_t center = body_get_centroid(player);
  if ((center.x - PLAYER_RADIUS) > WINDOW.x) {
    center.x -= WINDOW.x + 2 * PLAYER_RADIUS;
  } else if ((center.x + PLAYER_RADIUS) < 0) {
    center.x += WINDOW.x + 2 * PLAYER_RADIUS;
  }
  if ((center.y - PLAYER_RADIUS) > WINDOW.y) {
    center.y -= WINDOW.y + 2 * PLAYER_RADIUS;
  } else if ((center.y + PLAYER_RADIUS) < 0) {
    center.y += WINDOW.y + 2 * PLAYER_RADIUS;
  }
  body_set_centroid(player, center);
}

void coin_spawn(scene_t *scene) {
  if (rand() < (double)RAND_MAX * (PELLET_CHANCE * 6)) {
    vector_t center = randomize_center(scene);
    info_t *info = info_init(COIN);
    body_t *coin = make_rectangle(scene, center, COIN_COLOR, 20.0, 20.0, info->team, COIN_MASS, "assets/coin.png");
    scene_add_body(scene, coin);
    body_t *player = scene_get_body(scene, 1);
    create_coin_collecting(scene, player, coin);
  }
}

// creates the obstacles that cause the obstacle to change directions
void bouncing_spawn(scene_t *scene) {
  if (rand() < (double)RAND_MAX * (PELLET_CHANCE * 0.8)) {
    vector_t center = randomize_center(scene);
    info_t *info = info_init(OBSTACLE);
    body_t *new_bouncing = make_rectangle(scene, center, BOUNCING_OBSTACLE_COLOR, 50.0, 20.0, info->team, 100000000, "assets/bounce_obstacle_1.png");
    body_t *player = scene_get_body(scene, 1);
    vector_t vel = (vector_t){.x = 100, .y = 100};
    body_set_velocity(new_bouncing, vel);
    create_delete_bounce(scene, 1.5, new_bouncing,
                          player);
    body_t *rectangle1 = scene_get_body(scene, 3);
    body_t *rectangle2 = scene_get_body(scene, 4);
    body_t *rectangle3 = scene_get_body(scene, 5);
    body_t *rectangle4 = scene_get_body(scene, 6);
    scene_add_body(scene, new_bouncing);
    create_physics_collision(scene, OBSTACLE_ELASTICITY, rectangle1, new_bouncing);
    create_physics_collision(scene, OBSTACLE_ELASTICITY, rectangle2, new_bouncing);
    create_physics_collision(scene, OBSTACLE_ELASTICITY, rectangle3, new_bouncing);
    create_physics_collision(scene, OBSTACLE_ELASTICITY, rectangle4, new_bouncing);
    create_destructive_collision(scene, player, new_bouncing);
    }
}

// change to gravitational vortex that ends game?
void reduce_spawn(scene_t *scene) {
    if (rand() < (double)RAND_MAX * PELLET_CHANCE) {
      info_t *info = info_init(OBSTACLE);
      vector_t center = (vector_t) randomize_center(scene);
      body_t *obstacle = make_rectangle(scene, center, REDUCEVEL_COLOR, 40.0, 20.0, info->team, OBSTACLE_MASS, "assets/bounce_obstacle_2.png");
      body_t *player = scene_get_body(scene, 1);
      scene_add_body(scene, obstacle);
      create_vortex(scene, GRAVITY, player, obstacle);
      
}
}

void power_obstacle(scene_t *scene) {
  if (rand() < (double)RAND_MAX * (PELLET_CHANCE * 3)) {
    vector_t center = (vector_t) randomize_center(scene);
    info_t *info = info_init(OBSTACLE);
    body_t *obstacle = make_rectangle(scene, center, STALKERVEL_COLOR, 40.0, 20.0, info->team, OBSTACLE_MASS, "assets/bounce_obstacle_3.png");
    scene_add_body(scene, obstacle);
    body_t *stalker = scene_get_body(scene, 2);
    create_collision_velocity(scene, stalker, obstacle);
}
}

void is_game_over(scene_t *scene) {
  body_t *player = scene_get_body(scene, 1);
  info_t *info = (info_t *)body_get_info(player);
  if (info->team != ALLY_PLAYER) {
    outcome(scene, "assets/game_over_screen.jpeg");
  }
  
  if (body_get_coins(player) == 10) {
    outcome(scene, "assets/victory_screen.jpeg");
  }
}


state_t *emscripten_init() {
  state_t *state = malloc(sizeof(state_t));
  state->is_held = 0;
  state->last_dir_held = 3;
  time_t start_time = time(NULL);
  state->start_time = start_time; 
  vector_t min = (vector_t){.x = 0, .y = 0};
  sdl_init(min, WINDOW);
  // scene creation
  scene_t *scene = scene_init();
  sdl_on_key(on_key);
  state->scene = scene;
  make_background(scene, "assets/purple_background.png");
  make_player(scene);
  make_stalker(scene);
  initialize_walls(scene);
  return state;
}

void emscripten_main(state_t *state) {
  double dt = time_since_last_tick();
  scene_t *scene = state->scene;
  coin_spawn(scene);
  bouncing_spawn(scene);
  reduce_spawn(scene);
  power_obstacle(scene);
  wrap_around(scene, scene_get_body(scene, 1));
  scene_tick(scene, dt);
  time_t curr_time = time(NULL);
  time_t time_elapsed = curr_time - state->start_time;
  time_t time_left = TIMER - time_elapsed;
  // checks every tick if time ran out
  if (time_elapsed >= TIMER) {
        outcome(scene, "assets/game_over_screen.jpeg");
    }
  is_game_over(scene);
  sdl_render_scene(scene);
  sdl_render_text_time(time_left);
  body_t *player = scene_get_body(scene, 1);
  sdl_render_text_coins(player);
}

void emscripten_free(state_t *state) {
  scene_free(state->scene);
  free(state);
}
