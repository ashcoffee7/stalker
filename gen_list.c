#include "gen_list.h"
#include "vector.h"
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

typedef struct gen_list {
  size_t size;
  size_t capacity;
  void **arr;
} gen_list_t;

gen_list_t *gen_list_init(size_t initial_size) {
  gen_list_t *out = malloc(sizeof(gen_list_t));
  out->size = 0;
  out->capacity = initial_size;
  out->arr = malloc(sizeof(void *) * initial_size);
  return out;
}

void gen_list_resize(gen_list_t *curr) {
  void **out = malloc(sizeof(void *) * curr->capacity * 2);
  curr->capacity = curr->capacity * 2;
  for (size_t i = 0; i < curr->size; i++) {
    out[i] = curr->arr[i];
  }
  free(curr->arr);
  curr->arr = out;
}

void gen_list_free(gen_list_t *list, gen_free_t free_inator) {
  for (size_t i = 0; i < list->size; i++) {
    free_inator(list->arr[i]);
  }
  free(list->arr);
  free(list);
}

size_t gen_list_size(gen_list_t *list) { return list->size; }

void *gen_list_get(gen_list_t *list, size_t index) {
  assert(index < list->size);
  return list->arr[index];
}

void gen_list_add_back(gen_list_t *list, void *value) {
  assert(value != NULL);
  if (list->size >= list->capacity) {
    gen_list_resize(list);
  }
  list->arr[list->size] = value;
  list->size += 1;
}

void gen_list_add_front(gen_list_t *list, void *value) {
  assert(value != NULL);
  if (list->size >= list->capacity) {
    gen_list_resize(list);
  }
  for (size_t i = list->size; i > 0; i--) {
    list->arr[i] = list->arr[i - 1];
  }
  list->arr[0] = value;
  list->size += 1;
}

void *gen_list_remove_back(gen_list_t *list) {
  assert(list->size > 0);
  void *out = list->arr[list->size - 1];
  list->size -= 1;
  return out;
}

void *gen_list_remove_front(gen_list_t *list) {
  assert(list->size > 0);
  if (list->size == 1) {
    gen_list_remove_back(list);
  }
  void *out = list->arr[0];
  for (size_t i = 1; i < list->size; i++) {
    list->arr[i - 1] = list->arr[i];
  }
  list->size -= 1;
  return out;
}