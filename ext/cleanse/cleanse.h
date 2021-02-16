#ifndef _CLEANSE_H_RUBY
#define _CLEANSE_H_RUBY

#include <assert.h>
#include <ruby.h>
#include <ruby/encoding.h>
#include "strbuf.h"

#include "gumbo.h"
#include "parser.h"
#include "string_set.h"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
// #define likely(x) __builtin_expect((x), 1)
// #define unlikely(x) __builtin_expect((x), 0)
#define CSTR2SYM(s) (ID2SYM(rb_intern((s))))
#define OPTHASH_GIVEN_P(opts) \
  (argc > 0 && !NIL_P((opts) = rb_check_hash_type(argv[argc - 1])) && (--argc, 1))

static inline VALUE
strbuf_to_rb(GumboStringBuffer *buffer, bool do_free)
{
  VALUE rb_out = rb_enc_str_new(buffer->data, buffer->length, rb_utf8_encoding());
  if (do_free)
  {
    strbuf_free(buffer);
  }
  return rb_out;
}

enum
{
  CLEANSE_SANITIZER_ALLOW = (1 << 0),
  CLEANSE_SANITIZER_REMOVE_CONTENTS = (1 << 1),
  CLEANSE_SANITIZER_WRAP_WS = (1 << 2),
};

extern VALUE rb_mCleanse;
extern VALUE rb_cDocument;
extern VALUE rb_cDocumentFragment;
extern VALUE rb_cSanitizer;
extern VALUE rb_mConfig;
extern VALUE rb_cTextNode;
extern VALUE rb_cElementNode;

void cleanse_reparent_children_at(GumboNode *parent, GumboVector *new_children,
                                  unsigned int pos_at, bool wrap);
void cleanse_remove_child_at(GumboNode *parent, unsigned int pos_at, bool wrap);

void rb_cleanse_selector_check(VALUE rb_selector);
VALUE rb_cleanse_selector_coerce(VALUE rb_selector);
VALUE rb_cleanse_sanitizer_new(VALUE klass, VALUE rb_config);

void cleanse_set_element_flags(uint8_t *flags, VALUE rb_el, bool set, int flag);
GumboTag cleanse_rb_to_gumbo_tag(VALUE rb_tag);
void strcheck(VALUE rb_str);

void Init_cleanse(void);
void Init_cleanse_document(void);
void Init_cleanse_sanitizer(void);
void Init_cleanse_serializer(void);

typedef struct
{
  uint8_t flags[GUMBO_TAG_LAST];
  string_set_t attr_allowed;
  string_set_t class_allowed;
  st_table *element_sanitizers;
  int allow_comments : 1;
  int allow_doctype : 1;
} CleanseSanitizer;

typedef struct CleanseProtocolSanitizer
{
  char *name;
  string_set_t allowed;
  struct CleanseProtocolSanitizer *next;
} CleanseProtocolSanitizer;

typedef struct
{
  size_t max_nested;
  string_set_t attr_allowed;
  string_set_t attr_required;
  string_set_t class_allowed;
  CleanseProtocolSanitizer *protocols;
} CleanseElementSanitizer;

CleanseSanitizer *cleanse_sanitizer_new(void);
void cleanse_sanitizer_free(void *_sanitizer);
CleanseElementSanitizer *cleanse_sanitizer_get_element(CleanseSanitizer *sanitizer, GumboTag t);
CleanseProtocolSanitizer *cleanse_element_sanitizer_get_proto(
    CleanseElementSanitizer *elem, const char *proto);
void cleanse_node_sanitize(const CleanseSanitizer *sanitizer, GumboNode *node);

void cleanse_escape_html(GumboStringBuffer *out, const char *src,
                         long size, bool in_attribute);

VALUE cleanse_node_alloc(VALUE klass, VALUE rb_document, GumboNode *node);
VALUE cleanse_parse_to_rb(VALUE klass, VALUE rb_text, GumboTag fragment_ctx);
GumboOutput *cleanse_parse_fragment(VALUE rb_text, GumboTag fragment_ctx);

extern ID g_id_sanitizer;
extern ID g_id_html;

#endif
