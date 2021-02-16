#ifndef _CLEANSE_STRBUF_H
#define _CLEANSE_STRBUF_H

#include <string.h>
#include "string_buffer.h"
#include "macros.h"

#define strbuf GumboStringBuffer
// #define strbuf_put gumbo_string_buffer_put
// #define strbuf_puts gumbo_string_buffer_puts
// #define strbuf_putc gumbo_string_buffer_putc
// #define strbuf_putv gumbo_string_buffer_putv
// #define strbuf_init gumbo_string_buffer_init
// #define strbuf_free gumbo_string_buffer_destroy
#define strbuf_clear gumbo_string_buffer_clear
// #define strbuf_splice gumbo_string_buffer_splice

// TODO: toss

char *gumbo_strdup(const char *str) XMALLOC NONNULL_ARGS;

extern void *(*gumbo_user_allocator)(void *, size_t);
extern void (*gumbo_user_free)(void *);

static inline int gumbo_tolower(int c)
{
  return c | ((c >= 'A' && c <= 'Z') << 5);
}

void strbuf_init(GumboStringBuffer *output);

void strbuf_put(GumboStringBuffer *buffer,
                const char *data, size_t length);

static inline void strbuf_puts(GumboStringBuffer *buffer,
                               const char *data)
{
  strbuf_put(buffer, data, strlen(data));
}

void strbuf_splice(int where, int n_to_remove,
                   const char *data, int n_to_insert, GumboStringBuffer *buffer);

void strbuf_free(GumboStringBuffer *buffer);

void gumbo_element_set_attribute(
  GumboElement *element, const char *name, const char *value);

void gumbo_element_remove_attribute_at(GumboElement *element, unsigned int pos);

void gumbo_element_remove_attribute(GumboElement *element, const char *name);

void gumbo_vector_splice(
  int where, int n_to_remove, void **data, int n_to_insert, GumboVector *vector);

char *strdup(const char *str);

void gumbo_attribute_set_value(GumboAttribute *attr, const char *value);

void strbuf_putv(GumboStringBuffer *buffer, int count, ...);

void strbuf_putc(GumboStringBuffer *buffer, int c);

static inline void *gumbo_malloc(size_t size)
{
  return gumbo_user_allocator(NULL, size);
}

void gumbo_free(void *ptr);
void *gumbo_alloc(size_t size) XMALLOC;

#endif
