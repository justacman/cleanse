#include <stdarg.h>

#include "strbuf.h"

#include "attribute.h"
#include "parser.h"
#include "string_buffer.h"
#include "vector.h"
#include "util.h"

/*
 * Default memory management helpers;
 * set to system's realloc and free by default
 */
void *(* gumbo_user_allocator)(void *, size_t) = realloc;
void (* gumbo_user_free)(void *) = free;

void* gumbo_re_realloc(void* ptr, size_t size)
{
  ptr = realloc(ptr, size);
  if (unlikely(ptr == NULL)) {
    perror(__func__);
    abort();
  }
  return ptr;
}

static void maybe_resize_string_buffer(size_t additional_chars, GumboStringBuffer* buffer)
{
  size_t new_length = buffer->length + additional_chars;
  size_t new_capacity = buffer->capacity;
  while (new_capacity < new_length) {
    new_capacity *= 2;
  }
  if (new_capacity != buffer->capacity) {
    buffer->capacity = new_capacity;
    buffer->data = gumbo_re_realloc(buffer->data, buffer->capacity);
  }
}

void strbuf_init(GumboStringBuffer* output)
{
  output->data = gumbo_malloc(kDefaultStringBufferSize);
  output->length = 0;
  output->capacity = kDefaultStringBufferSize;
}

void strbuf_put(GumboStringBuffer *buffer,
                const char *data, size_t length)
{
  GumboStringPiece str = {
    data,
    length,
  };

  gumbo_string_buffer_append_string(&str, buffer);
}

void strbuf_putc(GumboStringBuffer *buffer, int c)
{
  size_t new_length = buffer->length + 1;
  size_t new_capacity = buffer->capacity;
  while (new_capacity < new_length) {
    new_capacity *= 2;
  }
  if (new_capacity != buffer->capacity) {
    buffer->capacity = new_capacity;
    buffer->data = gumbo_user_allocator(buffer->data, buffer->capacity);
  }

  maybe_resize_string_buffer(1, buffer);
  buffer->data[buffer->length++] = c;
}

void gumbo_string_buffer_splice(int where, int n_to_remove,
                                const char *data, int n_to_insert, GumboStringBuffer *buffer)
{
  size_t new_length = buffer->length + n_to_insert - n_to_remove;
  size_t new_capacity = buffer->capacity;
  while (new_capacity < new_length) {
    new_capacity *= 2;
  }
  if (new_capacity != buffer->capacity) {
    buffer->capacity = new_capacity;
    buffer->data = gumbo_user_allocator(buffer->data, buffer->capacity);
  }


  maybe_resize_string_buffer(n_to_insert - n_to_remove, buffer);
  memmove(buffer->data + where + n_to_insert,
          buffer->data + where + n_to_remove,
          (buffer->length - where - n_to_remove));
  memcpy(buffer->data + where, data, n_to_insert);
  buffer->length = buffer->length + n_to_insert - n_to_remove;
}

void strbuf_free(GumboStringBuffer* buffer)
{
  gumbo_free(buffer->data);
}

void gumbo_element_set_attribute(
  GumboElement *element, const char *name, const char *value)
{
  GumboVector *attributes = &element->attributes;
  GumboAttribute *attr = gumbo_get_attribute(attributes, name);

  if (!attr) {
    attr = gumbo_malloc(sizeof(GumboAttribute));
    attr->value = NULL;
    attr->attr_namespace = GUMBO_ATTR_NAMESPACE_NONE;

    attr->name = strdup(name);
    attr->original_name = kGumboEmptyString;
    attr->name_start = kGumboEmptySourcePosition;
    attr->name_end = kGumboEmptySourcePosition;

    gumbo_vector_add(attr, attributes);
  }

  gumbo_attribute_set_value(attr, value);
}

char* strdup(const char* str)
{
  const size_t size = strlen(str) + 1;
  // The strdup(3) function isn't available in strict "-std=c99" mode
  // (it's part of POSIX, not C99), so use malloc(3) and memcpy(3)
  // instead:
  char* buffer = gumbo_alloc(size);
  return memcpy(buffer, str, size);
}

void gumbo_element_remove_attribute_at(GumboElement *element, unsigned int pos)
{
  GumboAttribute *attr = element->attributes.data[pos];
  gumbo_vector_remove_at(pos, &element->attributes);
  gumbo_destroy_attribute(attr);
}

void gumbo_element_remove_attribute(GumboElement *element, const char *name)
{
  int idx = gumbo_get_attribute(&element->attributes, name);
  if (idx >= 0) {
    gumbo_element_remove_attribute_at(element, idx);
  }
}

// TODO
void* gumbo_realloc(void* ptr, size_t size) RETURNS_NONNULL;
static void enlarge_vector_if_full(GumboVector* vector, unsigned int space)
{
  unsigned int new_length = vector->length + space;
  unsigned int new_capacity = vector->capacity;

  if (!new_capacity) {
    // 0-capacity vector; no previous array to deallocate.
    vector->capacity = 2;
    vector->data = gumbo_alloc(sizeof(void*) * vector->capacity);
    return;
  }

  while (new_capacity < new_length) {
    new_capacity *= 2;
  }

  if (new_capacity != vector->capacity) {
    vector->capacity = new_capacity;
    size_t num_bytes = sizeof(void*) * vector->capacity;
    vector->data = gumbo_realloc(vector->data, num_bytes);
  }
}

void gumbo_vector_splice(int where, int n_to_remove,
                         void **data, int n_to_insert,
                         GumboVector* vector)
{
  enlarge_vector_if_full(vector, n_to_insert - n_to_remove);
  memmove(vector->data + where + n_to_insert,
          vector->data + where + n_to_remove,
          sizeof(void *) * (vector->length - where - n_to_remove));
  memcpy(vector->data + where, data, sizeof(void *) * n_to_insert);
  vector->length = vector->length + n_to_insert - n_to_remove;
}

void gumbo_string_buffer_put(GumboStringBuffer *buffer,
                             const char *data, size_t length)
{
  size_t new_length = buffer->length + length;
  size_t new_capacity = buffer->capacity;
  while (new_capacity < new_length) {
    new_capacity *= 2;
  }
  if (new_capacity != buffer->capacity) {
    buffer->capacity = new_capacity;
    buffer->data = gumbo_user_allocator(buffer->data, buffer->capacity);
  }

  maybe_resize_string_buffer(length, buffer);
  memcpy(buffer->data + buffer->length, data, length);
  buffer->length += length;
}


void gumbo_attribute_set_value(GumboAttribute *attr, const char *value)
{
  gumbo_free((void *)attr->value);
  attr->value = strdup(value);
  attr->original_value = kGumboEmptyString;
  attr->value_start = kGumboEmptySourcePosition;
  attr->value_end = kGumboEmptySourcePosition;
}

void strbuf_putv(GumboStringBuffer *buffer, int count, ...)
{
  va_list ap;
  int i;
  size_t total_len = 0;

  va_start(ap, count);
  for (i = 0; i < count; ++i) {
    total_len += strlen(va_arg(ap, const char *));
  }
  va_end(ap);

  size_t new_length = buffer->length + total_len;
  size_t new_capacity = buffer->capacity;
  while (new_capacity < new_length) {
    new_capacity *= 2;
  }
  if (new_capacity != buffer->capacity) {
    buffer->capacity = new_capacity;
    buffer->data = gumbo_user_allocator(buffer->data, buffer->capacity);
  }

  maybe_resize_string_buffer(total_len, buffer);

  va_start(ap, count);
  for (i = 0; i < count; ++i) {
    const char *data = va_arg(ap, const char *);
    size_t length = strlen(data);

    memcpy(buffer->data + buffer->length, data, length);
    buffer->length += length;
  }
  va_end(ap);
}
