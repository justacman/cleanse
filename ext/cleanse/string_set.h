#ifndef _CLEANSE_STRING_SET_H
#define _CLEANSE_STRING_SET_H

#include <stdbool.h>
#include <stdlib.h>

typedef struct {
  char **strings;
  uint32_t size;
  uint32_t allocated;
} string_set_t;

void string_set_new(string_set_t *set);
void string_set_add(string_set_t *set, const char *str);
void string_set_remove(string_set_t *set, const char *str);
bool string_set_contains(const string_set_t *set, const char *str);
void string_set_free(string_set_t *set);

#endif
