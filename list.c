#include "list.h"
#include "vector.h"
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

typedef struct list {
  size_t size;
  size_t capacity;
  void **arr;
  free_func_t free_inator;
} list_t;

list_t *list_init(size_t initial_size, free_func_t free_inator) {
  list_t *out = malloc(sizeof(list_t));
  assert(out != NULL);
  out->size = 0;
  out->capacity = initial_size;
  out->arr = malloc(sizeof(void *) * initial_size);
  out->free_inator = free_inator;
  return out;
}

void list_resize(list_t *curr) {
  void **out = malloc(sizeof(void *) * (curr->capacity * 2 + 1));
  curr->capacity = curr->capacity * 2 + 1;
  for (size_t i = 0; i < curr->size; i++) {
    out[i] = curr->arr[i];
  }
  free(curr->arr);
  curr->arr = out;
}

void list_free(list_t *list) {
  if (list->free_inator != NULL) {
    for (size_t i = 0; i < list->size; i++) {
      list->free_inator(list->arr[i]);
    }
  }
  free(list->arr);
  free(list);
}

size_t list_size(list_t *list) { return list->size; }

void *list_get(list_t *list, size_t index) {
  assert(index < list->size);
  return list->arr[index];
}

void list_add_back(list_t *list, void *value) {
  assert(value != NULL);
  if (list->size >= list->capacity) {
    list_resize(list);
  }
  list->arr[list->size] = value;
  list->size += 1;
}

void list_add_front(list_t *list, void *value) {
  assert(value != NULL);
  if (list->size >= list->capacity) {
    list_resize(list);
  }
  assert(list->size < list->capacity);
  for (size_t i = list->size; i > 0; i--) {
    list->arr[i] = list->arr[i - 1];
  }
  list->arr[0] = value;
  list->size += 1;
}

void list_add(list_t *list, void *value) { list_add_back(list, value); }

void *list_remove_back(list_t *list) {
  assert(list->size > 0);
  void *out = list->arr[list->size - 1];
  list->size -= 1;
  return out;
}

void list_replace(list_t *list, size_t index, void *value) {
  assert(value != NULL);
  assert(index < list->size);
  void *old = list_get(list, index);
  list->free_inator(old);
  list->arr[index] = value;
  return;
}

void *list_remove_front(list_t *list) {
  assert(list->size > 0);
  if (list->size == 1) {
    return list_remove_back(list);
  }
  void *out = list->arr[0];
  for (size_t i = 1; i < list->size; i++) {
    list->arr[i - 1] = list->arr[i];
  }
  list->size -= 1;
  return out;
}

void *list_remove(list_t *list, size_t idx) {
  assert(idx < list->size);
  if (list->size == 1) {
    return list_remove_back(list);
  }
  void *out = list->arr[idx];
  for (size_t i = idx + 1; i < list->size; i++) {
    list->arr[i - 1] = list->arr[i];
  }
  list->size -= 1;
  return out;
}

size_t list_find(list_t *list, void *item) {
  for (size_t i = 0; i < list_size(list); i++) {
    void *curr = list_get(list, i);
    if (curr == item) {
      return i;
    }
  }
  return -1;
}