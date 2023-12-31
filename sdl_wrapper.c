#include "sdl_wrapper.h"
#include "list.h"
#include "scene.h"
#include "state.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

const char WINDOW_TITLE[] = "CS 3";
const int WINDOW_WIDTH = 1000;
const int WINDOW_HEIGHT = 500;
const double MS_PER_S = 1e3;

// sound constants
const size_t FREQUENCY = 44100; // default for music sampling
const size_t STEREO = 2;
const size_t CHUNK_SIZE = 4096; // about 100ms

/**
 * The coordinate at the center of the screen.
 */
vector_t center;
/**
 * The coordinate difference from the center to the top right corner.
 */
vector_t max_diff;
/**
 * The SDL window where the scene is rendered.
 */
SDL_Window *window;
/**
 * The renderer used to draw the scene.
 */
SDL_Renderer *renderer;
/**
 * The keypress handler, or NULL if none has been configured.
 */
key_handler_t key_handler = NULL;
/**
 * SDL's timestamp when a key was last pressed or released.
 * Used to mesasure how long a key has been held.
 */
uint32_t key_start_timestamp;
/**
 * The value of clock() when time_since_last_tick() was last called.
 * Initially 0.
 */
clock_t last_clock = 0;
list_t *textures;
list_t *music_tracks;
list_t *sound_effects;

/** Computes the center of the window in pixel coordinates */
vector_t get_window_center(void) {
  int *width = malloc(sizeof(*width)), *height = malloc(sizeof(*height));
  assert(width != NULL);
  assert(height != NULL);
  SDL_GetWindowSize(window, width, height);
  vector_t dimensions = {.x = *width, .y = *height};
  free(width);
  free(height);
  return vec_multiply(0.5, dimensions);
}

/**
 * Computes the scaling factor between scene coordinates and pixel coordinates.
 * The scene is scaled by the same factor in the x and y dimensions,
 * chosen to maximize the size of the scene while keeping it in the window.
 */
double get_scene_scale(vector_t window_center) {
  // Scale scene so it fits entirely in the window
  double x_scale = window_center.x / max_diff.x,
         y_scale = window_center.y / max_diff.y;
  return x_scale < y_scale ? x_scale : y_scale;
}

/** Maps a scene coordinate to a window coordinate */
vector_t get_window_position(vector_t scene_pos, vector_t window_center) {
  // Scale scene coordinates by the scaling factor
  // and map the center of the scene to the center of the window
  vector_t scene_center_offset = vec_subtract(scene_pos, center);
  double scale = get_scene_scale(window_center);
  vector_t pixel_center_offset = vec_multiply(scale, scene_center_offset);
  vector_t pixel = {.x = round(window_center.x + pixel_center_offset.x),
                    // Flip y axis since positive y is down on the screen
                    .y = round(window_center.y - pixel_center_offset.y)};
  return pixel;
}

/**
 * Converts an SDL key code to a char.
 * 7-bit ASCII characters are just returned
 * and arrow keys are given special character codes.
 */
char get_keycode(SDL_Keycode key) {
  switch (key) {
  case SDLK_LEFT:
    return LEFT_ARROW;
  case SDLK_UP:
    return UP_ARROW;
  case SDLK_RIGHT:
    return RIGHT_ARROW;
  case SDLK_DOWN:
    return DOWN_ARROW;
  case SDLK_SPACE:
    return SPACE_BAR;
  case SDLK_w:
    return W_KEY;
  case SDLK_a:
    return A_KEY;
  case SDLK_s:
    return S_KEY;
  case SDLK_d:
    return D_KEY;
  default:
    // Only process 7-bit ASCII characters
    return key == (SDL_Keycode)(char)key ? key : '\0';
  }
}

void sdl_init(vector_t min, vector_t max) {
  TTF_Init();
  // Check parameters
  assert(min.x < max.x);
  assert(min.y < max.y);
  center = vec_multiply(0.5, vec_add(min, max));
  max_diff = vec_subtract(max, center);
  SDL_Init(SDL_INIT_EVERYTHING);
  window = SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_CENTERED,
                            SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT,
                            SDL_WINDOW_RESIZABLE);
  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
  textures = list_init(2600, (free_func_t)(free));
  SDL_Texture *texture_bounce = SDL_CreateTextureFromSurface(renderer, IMG_Load("assets/bounce_obstacle_1.png"));
  SDL_Texture *texture_coin = SDL_CreateTextureFromSurface(renderer, IMG_Load("assets/coin.png"));
  SDL_Texture *texture_player = SDL_CreateTextureFromSurface(renderer, IMG_Load("assets/player_down.png"));
  SDL_Texture *texture_stalker = SDL_CreateTextureFromSurface(renderer, IMG_Load("assets/stalker_up.png"));
  SDL_Texture *texture_vel = SDL_CreateTextureFromSurface(renderer, IMG_Load("assets/bounce_obstacle_2.png"));
  SDL_Texture *texture_power = SDL_CreateTextureFromSurface(renderer, IMG_Load("assets/bounce_obstacle_3.png"));
  SDL_Texture *texture_background = SDL_CreateTextureFromSurface(renderer, IMG_Load("assets/purple_background.png"));
  SDL_Texture *texture_win = SDL_CreateTextureFromSurface(renderer, IMG_Load("assets/victory_screen.jpeg"));
  SDL_Texture *texture_loose = SDL_CreateTextureFromSurface(renderer, IMG_Load("assets/game_over_screen.jpeg"));
  list_add(textures, texture_bounce);
  list_add(textures, texture_coin);
  list_add(textures, texture_player);
  list_add(textures, texture_stalker);
  list_add(textures, texture_vel);
  list_add(textures, texture_power);
  list_add(textures, texture_background);
  list_add(textures, texture_win);
  list_add(textures, texture_loose);

  // initialize SDL
  SDL_Init(SDL_INIT_AUDIO);
  Mix_OpenAudio(FREQUENCY, MIX_DEFAULT_FORMAT, STEREO, CHUNK_SIZE);
  // music
  Mix_Chunk *normal_music = Mix_LoadWAV("assets/Space_Search.wav");
  assert(normal_music != NULL);
  music_tracks = list_init(1, (free_func_t)(Mix_FreeChunk));
  list_add(music_tracks, normal_music);

  // sfx
  Mix_Chunk *dash = Mix_LoadWAV("assets/Pharah_DUPE_1.wav");
  Mix_Chunk *victory = Mix_LoadWAV("assets/Hooray.wav");
  Mix_Chunk *defeat = Mix_LoadWAV("assets/gg_go_next.wav");
  sound_effects = list_init(3, (free_func_t)(Mix_FreeChunk));
  list_add(sound_effects, dash);
  list_add(sound_effects, victory);
  list_add(sound_effects, defeat);
}

bool sdl_is_done(state_t *state) {
  SDL_Event *event = malloc(sizeof(*event));
  assert(event != NULL);
  while (SDL_PollEvent(event)) {
    switch (event->type) {
    case SDL_QUIT:
      free(event);
      return true;
    case SDL_KEYDOWN:
    case SDL_KEYUP:
      // Skip the keypress if no handler is configured
      // or an unrecognized key was pressed
      if (key_handler == NULL)
        break;
      char key = get_keycode(event->key.keysym.sym);
      if (key == '\0')
        break;

      uint32_t timestamp = event->key.timestamp;
      if (!event->key.repeat) {
        key_start_timestamp = timestamp;
      }
      key_event_type_t type =
          event->type == SDL_KEYDOWN ? KEY_PRESSED : KEY_RELEASED;
      double held_time = (timestamp - key_start_timestamp) / MS_PER_S;
      key_handler(key, type, held_time, state);
      break;
    }
  }
  free(event);
  return false;
}

void sdl_clear(void) {
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
  SDL_RenderClear(renderer);
}

void sdl_draw_polygon(list_t *points, rgb_color_t color) {
  // Check parameters
  size_t n = list_size(points);
  assert(n >= 3);
  assert(0 <= color.r && color.r <= 1);
  assert(0 <= color.g && color.g <= 1);
  assert(0 <= color.b && color.b <= 1);

  vector_t window_center = get_window_center();

  // Convert each vertex to a point on screen
  int16_t *x_points = malloc(sizeof(*x_points) * n),
          *y_points = malloc(sizeof(*y_points) * n);
  assert(x_points != NULL);
  assert(y_points != NULL);
  for (size_t i = 0; i < n; i++) {
    vector_t *vertex = list_get(points, i);
    vector_t pixel = get_window_position(*vertex, window_center);
    x_points[i] = pixel.x;
    y_points[i] = pixel.y;
  }

  // Draw polygon with the given color
  filledPolygonRGBA(renderer, x_points, y_points, n, color.r * 255,
                    color.g * 255, color.b * 255, 255);
  free(x_points);
  free(y_points);
}

void sdl_show(void) {
  // Draw boundary lines
  vector_t window_center = get_window_center();
  vector_t max = vec_add(center, max_diff),
           min = vec_subtract(center, max_diff);
  vector_t max_pixel = get_window_position(max, window_center),
           min_pixel = get_window_position(min, window_center);
  SDL_Rect *boundary = malloc(sizeof(*boundary));
  boundary->x = min_pixel.x;
  boundary->y = max_pixel.y;
  boundary->w = max_pixel.x - min_pixel.x;
  boundary->h = min_pixel.y - max_pixel.y;
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderDrawRect(renderer, boundary);
  free(boundary);

  SDL_RenderPresent(renderer);
}

void make_textures_list(scene_t *scene) {
  for (size_t i = 0; i < scene_bodies(scene); i++) {
    body_t *curr = scene_get_body(scene, i);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, IMG_Load(body_get_texture(curr)));
    list_add(textures, texture);
  }
}
 
void sdl_render_scene(scene_t *scene) {
  sdl_clear();
  // iterate over bodies; 
  for (size_t i = 0; i < scene_bodies(scene); i++) {
    body_t *curr = scene_get_body(scene,i);
    if (body_get_texture(curr) != NULL) {
      char* file = body_get_texture(curr);
      vector_t center = body_get_centroid(curr);
      SDL_Texture *texture;
      if (strcmp(file, "assets/bounce_obstacle_1.png") == 0) {
          texture = list_get(textures, 0);
      }
      else if (strcmp(file, "assets/coin.png") == 0) {
        texture = list_get(textures, 1);
      }
      else if (strcmp(file, "assets/player_down.png") == 0) {
        texture = list_get(textures, 2);
      }
      else if (strcmp(file, "assets/stalker_up.png") == 0) {
        texture = list_get(textures, 3);
      }  
      else if (strcmp(file, "assets/bounce_obstacle_2.png") == 0) {
        texture = list_get(textures, 4);
      }
      else if (strcmp(file, "assets/bounce_obstacle_3.png") == 0) {
        texture = list_get(textures, 5);
      }
      else if (strcmp(file, "assets/purple_background.png") == 0) {
        texture = list_get(textures, 6);
      }
      else if (strcmp(file, "assets/victory_screen.jpeg") == 0) {
        texture = list_get(textures, 7);
      }
      else if (strcmp(file, "assets/game_over_screen.jpeg") == 0) {
        texture = list_get(textures, 8);
      }
      // texture = list_get(textures, 1);
      SDL_Rect texture_rect;
      texture_rect.x = center.x;
      texture_rect.y = center.y;
      if ((strcmp(file, "assets/stalker_up.png") == 0) || (strcmp(file, "assets/player_down.png") == 0) || (strcmp(file, "assets/coin.png") == 0)) {
        texture_rect.w = 50.0;
        texture_rect.h = 50.0;
      }
      else if ((strcmp(file, "assets/purple_background.png") == 0) || (strcmp(file, "assets/victory_screen.jpeg") == 0) || (strcmp(file, "assets/game_over_screen.jpeg") == 0)) {
        texture_rect.w = WINDOW_WIDTH - 1;
        texture_rect.h = WINDOW_HEIGHT - 1;
      }
      else {
        texture_rect.w = 100.0;
        texture_rect.h = 100.0;
      }
  
      SDL_RenderCopy(renderer, texture, NULL, &texture_rect); 
      SDL_RenderPresent(renderer);
    }
    else {
      list_t *shape = body_get_shape(curr);
      sdl_draw_polygon(shape, body_get_color(curr));
      list_free(shape);
  }
  
  sdl_show();
}
}

void sdl_render_text_time(time_t time_left){
  TTF_Font* space_font = TTF_OpenFont("assets/AlienRavager-0WYod.ttf", 24);
  SDL_Color White = {255, 255, 255};
  size_t timer_text = (size_t)time_left;
  char *time_statement;
  sprintf(time_statement, "%zu seconds", timer_text);
  SDL_Surface* surface_message =
      TTF_RenderText_Solid(space_font, time_statement, White);

  SDL_Texture* message = SDL_CreateTextureFromSurface(renderer, surface_message);

  SDL_Rect message_rect;
  message_rect.x = 5;
  message_rect.y = 0;
  message_rect.w = 250;
  message_rect.h = 50;

  SDL_RenderCopy(renderer, message, NULL, &message_rect);
  SDL_FreeSurface(surface_message);
  SDL_DestroyTexture(message);

  sdl_show();
}

 void sdl_render_text_coins(body_t *body){
  TTF_Font* space_font = TTF_OpenFont("assets/AlienRavager-0WYod.ttf", 24);
  SDL_Color white = {255, 255, 255};

  char *time_statement;
  sprintf(time_statement, "%zu coins", body_get_coins(body));
  SDL_Surface* surface_message =
      TTF_RenderText_Solid(space_font, time_statement, white);

  SDL_Texture* message = SDL_CreateTextureFromSurface(renderer, surface_message);

  SDL_Rect message_rect;
  message_rect.x = 5;
  message_rect.y = 60;
  message_rect.w = 250; 
  message_rect.h = 50; 

  SDL_RenderCopy(renderer, message, NULL, &message_rect);
  SDL_FreeSurface(surface_message);
  SDL_DestroyTexture(message);

  sdl_show();
}

// referencing https://wiki.libsdl.org/SDL2_mixer/CategoryAPI
// -1 = error, 0 = okay
int sdl_render_music() {
  Mix_Chunk *music = list_get(music_tracks, 0);

  // check that the music objects isn't null
  if (music == NULL) {
    return -1;
  }

  if (Mix_PlayChannel(-1, music, 0) == -1) {
      return -1;
  }

  return 0;
}

int sdl_render_sfx(int sound_to_make) {
  // sfxs
  Mix_Chunk *dash = list_get(sound_effects, 0);
  Mix_Chunk *victory = list_get(sound_effects, 1);
  Mix_Chunk *defeat = list_get(sound_effects, 2);

  // check that the sfx aren't null
  if (dash == NULL || victory == NULL || defeat == NULL) {
        return -1;
  }

  switch (sound_to_make) {
    case 5:
      if (Mix_PlayChannel(-1, dash, 0) == -1) {
        return -1;
      }      
      break;
    case 6:
      Mix_HaltChannel(-1);
      if (Mix_PlayChannel(-1, victory, 0) == -1) {
        return -1;
      }      
      break;
    case 7:
      Mix_HaltChannel(-1);
      if (Mix_PlayChannel(-1, defeat, 0) == -1) {
        return -1;
      }      
      break;
  }

  return 0;
}

void sdl_on_key(key_handler_t handler) { key_handler = handler; }

double time_since_last_tick(void) {
  clock_t now = clock();
  double difference = last_clock
                          ? (double)(now - last_clock) / CLOCKS_PER_SEC
                          : 0.0; // return 0 the first time this is called
  last_clock = now;
  return difference;
}


