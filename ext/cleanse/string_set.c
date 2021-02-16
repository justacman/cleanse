#include <ruby/util.h>
#include "cleanse.h"

#define FNV_SEED ((uint32_t)0x811c9dc5)

static uint32_t fnv_32a_str(char *str, uint32_t hval)
{
  unsigned char *s = (unsigned char *)str;	/* unsigned string */

  /*
   * FNV-1a hash each octet in the buffer
   */
  while (*s) {

    /* xor the bottom with the current octet */
    hval ^= (uint32_t)*s++;

    /* multiply by the 32 bit FNV magic prime mod 2^32 */
    hval += (hval<<1) + (hval<<4) + (hval<<7) + (hval<<8) + (hval<<24);
  }

  /* return our new hash value */
  return hval;
}

void string_set_new(string_set_t *set)
{
  short initial_size = 1;
  set->size = 0;
  set->allocated = 1;
  set->strings = (char **) calloc(1, sizeof(char *));
}

void string_set_free(string_set_t *set)
{
  for (size_t i = 0; i < set->size; i++) {
    free(set->strings[i]);
  }
  free(set->strings);
}

static void string_set_resize(string_set_t *set)
{
  uint32_t i, new_size = set->allocated ? (set->allocated * 2) : 4;
  char **new_strings = xcalloc(new_size, sizeof(char *));

  for (i = 0; i < set->allocated; ++i) {
    char *str = set->strings[i];

    if (str) {
      uint32_t hash = fnv_32a_str(str, FNV_SEED);

      while (new_strings[hash & (new_size - 1)] != NULL) {
        hash++;
      }

      new_strings[hash & (new_size - 1)] = str;
    }
  }

  xfree(set->strings);
  set->strings = new_strings;
  set->allocated = new_size;
}

void string_set_add(string_set_t *set, const char *str)
{
  if (string_set_contains(set, str)) {
    return;
  }

  uint32_t hash = fnv_32a_str(str, FNV_SEED);
  const char *m;

  if (set->size + 1 > set->allocated * 3 / 4) {
    string_set_resize(set);
  }

  while ((m = set->strings[hash & (set->allocated - 1)]) != NULL) {
    if (!strcmp(m, str)) {
      return;
    }
    hash++;
  }

  set->strings[hash & (set->allocated - 1)] = strdup(str);
  set->size++;
}

void string_set_remove(string_set_t *set, const char *str)
{
  uint32_t hash = fnv_32a_str(str, FNV_SEED);
  char *m;

  if (!set->allocated) {
    return;
  }

  while ((m = set->strings[hash & (set->allocated - 1)]) != NULL) {
    if (!strcmp(m, str)) {
      xfree(m);
      set->strings[hash & (set->allocated - 1)] = NULL;
      set->size--;
      return;
    }

    hash++;
  }
}

bool string_set_contains(const string_set_t *set, const char *str)
{
  uint32_t hash = fnv_32a_str(str, FNV_SEED);
  const char *m;

  if (!set->allocated) {
    return false;
  }

  while ((m = set->strings[hash & (set->allocated - 1)]) != NULL) {
    if (m[0] == str[0] && !strcmp(m + 1, str + 1)) {
      return true;
    }

    hash++;
  }

  return false;
}
