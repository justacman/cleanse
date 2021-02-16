#include "cleanse.h"
#include "cleanse_tag_helper.h"

#include "gumbo.h"

static VALUE rb_cSerializer;

enum {
  BEFORE_BEGIN = 0,
  AFTER_BEGIN = 1,
  BEFORE_END = 2,
  AFTER_END = 3
};

typedef struct {
  VALUE rb_result;
  VALUE rb_adjacent[4];
} CleanseReplace;

typedef struct {
  VALUE rb_document;
  VALUE rb_current_text;
  GumboStringBuffer current_text;
  GumboStringBuffer out;
} CleanseSerializer;

static void
serialize_node(GumboStringBuffer *out, CleanseSerializer *serial, GumboNode *node);

static GumboTag
fragment_context_for_node(CleanseSerializer *serial, GumboNode *node)
{
  assert(serial);

  if (node->parent && node->parent->type == GUMBO_NODE_DOCUMENT) {
    GumboOutput *document = DATA_PTR(serial->rb_document);
    GumboTag tag = document->root->v.element.tag;
    if (tag != GUMBO_TAG_LAST) {
      return tag;
    }
    // TODO
    // if (document->fragment_context != GUMBO_TAG_LAST)
    // return document->fragment_context;
  }

  return node->v.element.tag;
}

void
cleanse_escape_html(GumboStringBuffer *out, const char *src,
                    long size, bool in_attribute)
{
  static const char HTML_ESCAPE_TABLE[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 4, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  };

  static const char *HTML_ESCAPES[] = {
    "",
    "&quot;",
    "&amp;",
    "&lt;",
    "&gt;"
  };

  long i = 0, org, esc = 0;
  const uint8_t *keys = (const uint8_t *)src;

  for (i = 0; i < size; ++i) {
    org = i;
    while (i < size && (esc = HTML_ESCAPE_TABLE[(int)keys[i]]) == 0) {
      i++;
    }

    if (i > org) {
      strbuf_put(out, src + org, i - org);
    }

    /* escaping */
    if (unlikely(i >= size)) {
      break;
    }

    /*
     * HTML spec says '"' should not be
     * escaped unless inside an attribute
     */
    if (!in_attribute && keys[i] == '"') {
      strbuf_put(out, "\"", 1);
      continue;
    }

    strbuf_puts(out, HTML_ESCAPES[esc]);
  }
}


static void
cleanse_tag_name_serialize(GumboStringBuffer *out, GumboElement *element)
{
  assert(element->tag <= GUMBO_TAG_LAST);

  if (element->tag == GUMBO_TAG_UNKNOWN) {
    /*
                 * Gumbo's tokenizer lowercases tag names but then throws them
                 * away. If they were preserved we wouldn't have to do our own
                 * lowercasing here.
     */

    unsigned int x;
    GumboStringPiece tag_name = element->original_tag;
    gumbo_tag_from_original_text(&tag_name);

    gumbo_string_buffer_reserve(out->length + tag_name.length, out);
    for (x = 0; x < tag_name.length; ++x) {
      out->data[out->length + x] = gumbo_tolower(tag_name.data[x]);
    }
    out->length += tag_name.length;
  } else {
    strbuf_puts(out, gumbo_normalized_tagname(element->tag));
  }
}

static void
cleanse_start_tag_serialize(GumboStringBuffer *out, GumboElement *element)
{
  GumboVector *attributes = &element->attributes;
  unsigned int x;

  strbuf_put(out, "<", 1);
  cleanse_tag_name_serialize(out, element);

  for (x = 0; x < attributes->length; ++x) {
    GumboAttribute *attr = attributes->data[x];

    strbuf_putv(out, 3, " ", attr->name, "=\"");
    cleanse_escape_html(out, attr->value, strlen(attr->value), true);
    strbuf_put(out, "\"", 1);
  }

  strbuf_put(out, ">", 1);
}

static VALUE
update_current_text(CleanseSerializer *serial, GumboNode *node)
{
  GumboStringBuffer *escaped_text = &serial->current_text;
  VALUE rb_text = serial->rb_current_text;

  strbuf_clear(escaped_text);
  cleanse_escape_html(escaped_text, node->v.text.text, strlen(node->v.text.text), false);

  DATA_PTR(rb_text) = node;
  rb_ivar_set(rb_text, g_id_html, strbuf_to_rb(escaped_text, false));

  return rb_text;
}

static void
serialize_replacement_node(GumboStringBuffer *out, CleanseSerializer *serial,
                           GumboNode *node, VALUE rb_adjacent[]);

static void
process_text_node_1(GumboStringBuffer *out, CleanseSerializer *serial,
                    long n, GumboNode *node, GumboNode *parent)
{
  VALUE rb_result = Qnil;
  VALUE rb_node = update_current_text(serial, node);
  CleanseReplace replace = {rb_node, {Qnil, Qnil, Qnil, Qnil}};
  GumboOutput *replace_with_fragment = NULL;
  VALUE rb_result_class;

  if (replace.rb_result == Qfalse) {
    return;
  }

  if (rb_type(replace.rb_result) == T_STRING) {
    strcheck(replace.rb_result);
    replace.rb_result = cleanse_parse_to_rb(
                          rb_cDocumentFragment, replace.rb_result,
                          fragment_context_for_node(serial, node->parent));
  }

  Check_Type(replace.rb_result, T_DATA);
  rb_result_class = rb_class_of(replace.rb_result);

  /*
   * Otherwise, check if we got a new Node object,
   * or a whole document fragment
   */
  if (rb_result_class == rb_cElementNode || rb_result_class == rb_cTextNode) {
    node = DATA_PTR(replace.rb_result);
  } else if (rb_result_class == rb_cDocumentFragment) {
    replace_with_fragment = DATA_PTR(replace.rb_result);
  } else {
    rb_raise(rb_eTypeError,
             "Expected a String, Node, or DocumentFragment as replacement "
             "for the existing node");
  }

  /*
   * Iterate through the returned fragment and apply the remaining filters
   * to each chunk of text in it
   */
  if (replace_with_fragment) {
    unsigned int i;
    GumboVector *children = &replace_with_fragment->root->v.element.children;

    for (i = 0; i < children->length; ++i) {
      GumboNode *child = children->data[i];
      if (child->type == GUMBO_NODE_TEXT) {
        process_text_node_1(out, serial, n + 1, child, parent);
      } else {
        serialize_node(out, NULL, child);
      }
    }
  } else {
    serialize_replacement_node(out, serial, node, replace.rb_adjacent);
  }
}

static void
process_text_node(GumboStringBuffer *out, CleanseSerializer *serial, GumboNode *node)
{
  process_text_node_1(out, serial, 0, node, node->parent);
}

static void
serialize_adjacent(GumboStringBuffer *out, VALUE rb_adjacent, GumboTag fragment_ctx)
{
  GumboOutput *output;
  GumboVector *children;
  unsigned int x;

  strcheck(rb_adjacent);

  output = cleanse_parse_fragment(rb_adjacent, fragment_ctx);
  children = &output->root->v.element.children;

  for (x = 0; x < children->length; ++x) {
    serialize_node(out, NULL, children->data[x]);
  }

  gumbo_destroy_output(output);
}

static void
serialize_replacement_element(GumboStringBuffer *out, CleanseSerializer *serial,
                              GumboNode *node, VALUE rb_adjacent[])
{
  GumboElement *element = &node->v.element;

  if (!NIL_P(rb_adjacent[BEFORE_BEGIN])) {
    serialize_adjacent(
      out, rb_adjacent[BEFORE_BEGIN],
      fragment_context_for_node(serial, node->parent));
  }

  cleanse_start_tag_serialize(out, element);

  if (!element_is_void(element->tag)) {
    if (node->type != GUMBO_NODE_TEMPLATE) {
      GumboVector *children = &element->children;
      unsigned int x;

      if (!NIL_P(rb_adjacent[AFTER_BEGIN])) {
        serialize_adjacent(out, rb_adjacent[AFTER_BEGIN], element->tag);
      }

      for (x = 0; x < children->length; ++x) {
        serialize_node(out, serial, children->data[x]);
      }

      if (!NIL_P(rb_adjacent[BEFORE_END])) {
        serialize_adjacent(out, rb_adjacent[BEFORE_END], element->tag);
      }
    }

    strbuf_put(out, "</", 2);
    cleanse_tag_name_serialize(out, element);
    strbuf_put(out, ">", 1);
  }

  if (!NIL_P(rb_adjacent[AFTER_END])) {
    serialize_adjacent(
      out, rb_adjacent[AFTER_END],
      fragment_context_for_node(serial, node->parent));
  }
}

static void
serialize_replacement_node(GumboStringBuffer *out, CleanseSerializer *serial,
                           GumboNode *node, VALUE rb_adjacent[])
{
  switch (node->type) {
  case GUMBO_NODE_DOCUMENT:
    rb_raise(rb_eRuntimeError, "unexpected Document node");
    break;
  case GUMBO_NODE_COMMENT:
    rb_raise(rb_eRuntimeError, "unexpected Comment node");
    break;
  case GUMBO_NODE_ELEMENT:
  case GUMBO_NODE_TEMPLATE:
    serialize_replacement_element(out, serial, node, rb_adjacent);
    break;
  case GUMBO_NODE_TEXT:
  case GUMBO_NODE_CDATA:
  case GUMBO_NODE_WHITESPACE:
    serialize_node(out, NULL, node);
    break;
  }
}

static void
serialize_element(GumboStringBuffer *out,
                  CleanseSerializer *serial, GumboNode *node)
{
  CleanseReplace replace = {Qnil, {Qnil, Qnil, Qnil, Qnil}};

  serialize_replacement_node(out, serial, node, replace.rb_adjacent);
}

static void
serialize_node(GumboStringBuffer *out, CleanseSerializer *serial, GumboNode *node)
{
  switch (node->type) {
  case GUMBO_NODE_DOCUMENT:
    rb_raise(rb_eRuntimeError, "unexpected Document node");
    break;

  case GUMBO_NODE_ELEMENT:
  case GUMBO_NODE_TEMPLATE:
    serialize_element(out, serial, node);
    break;

  case GUMBO_NODE_WHITESPACE: {
    GumboText *text = &node->v.text;
    strbuf_puts(out, text->text);
    break;
  }

  case GUMBO_NODE_TEXT:
  case GUMBO_NODE_CDATA: {
    GumboText *text = &node->v.text;
    GumboNode *parent = node->parent;

    assert(parent->type == GUMBO_NODE_ELEMENT);

    if (element_is_rcdata(parent->v.element.tag)) {
      strbuf_puts(out, text->text);
    } else if (serial) {
      process_text_node(out, serial, node);
    } else {
      cleanse_escape_html(out, text->text, strlen(text->text), false);
    }
    break;
  }

  case GUMBO_NODE_COMMENT: {
    GumboText *text = &node->v.text;
    strbuf_putv(out, 3, "<!--", text->text, "-->");
    break;
  }

  default:
    rb_raise(rb_eRuntimeError, "unimplemented");
  }
}

static void
serialize_document(GumboStringBuffer *out, CleanseSerializer *serial,
                   GumboDocument *document, bool add_doctype)
{
  GumboVector *children = &document->children;
  unsigned int i;

  if (document->has_doctype) {
    strbuf_putv(out, 2, "<!DOCTYPE ", document->name);
    if (document->public_identifier && document->public_identifier[0]) {
      strbuf_putv(out, 3,
                  " PUBLIC \"", document->public_identifier, "\"");
      if (document->system_identifier && document->system_identifier[0])
        strbuf_putv(out, 3,
                    " \"", document->system_identifier, "\"");
    } else if (document->system_identifier && document->system_identifier[0]) {
      strbuf_putv(out, 3,
                  " SYSTEM \"", document->system_identifier, "\"");
    }
    strbuf_put(out, ">", 1);
  } else if (add_doctype) {
    strbuf_puts(out,
                "<!DOCTYPE html>"
               );
  }

  for (i = 0; i < children->length; ++i) {
    GumboNode *child = children->data[i];
    serialize_node(out, serial, child);
  }
}

static void
rb_cleanse_serializer_free(CleanseSerializer *serial)
{
  strbuf_free(&serial->out);
  strbuf_free(&serial->current_text);
}

static void
rb_cleanse_serializer_mark(CleanseSerializer *serial)
{
  rb_gc_mark(serial->rb_document);
  rb_gc_mark(serial->rb_current_text);
}

static VALUE
rb_cleanse_serializer_to_html(VALUE rb_self)
{
  VALUE rb_result, rb_document, rb_sanitizer;
  bool allow_doctype;
  CleanseSerializer *serial = NULL;
  CleanseSanitizer *sanitizer = NULL;
  GumboOutput *output = NULL;

  Data_Get_Struct(rb_self, CleanseSerializer, serial);

  rb_document = serial->rb_document;
  Data_Get_Struct(rb_document, GumboOutput, output);

  if (rb_obj_is_kind_of(rb_document, rb_cDocumentFragment)) {
    GumboVector *children = &output->root->v.element.children;
    unsigned int x;
    for (x = 0; x < children->length; ++x) {
      serialize_node(&serial->out, serial, children->data[x]);
    }
  } else {
    rb_sanitizer = rb_ivar_get(rb_document, g_id_sanitizer);
    if (!rb_obj_is_kind_of(rb_sanitizer, rb_cSanitizer)) {
      rb_raise(rb_eTypeError, "expected a Cleanse::Sanitizer instance");
    }
    Data_Get_Struct(rb_sanitizer, CleanseSanitizer, sanitizer);
    allow_doctype = sanitizer->allow_doctype;

    serialize_document(&serial->out, serial, &output->document->v.document, allow_doctype);
  }

  rb_result = strbuf_to_rb(&serial->out, false);

  return rb_result;
}

static VALUE rb_cleanse_serializer_new(VALUE rb_klass, VALUE rb_document)
{
  VALUE rb_serializer;
  CleanseSerializer *serial = NULL;

  rb_serializer = Data_Make_Struct(rb_klass, CleanseSerializer,
                                   rb_cleanse_serializer_mark, rb_cleanse_serializer_free, serial);

  strbuf_init(&serial->out);
  strbuf_init(&serial->current_text);

  serial->rb_document = rb_document;
  serial->rb_current_text = cleanse_node_alloc(rb_cTextNode, rb_document, NULL);

  return rb_serializer;
}

void Init_cleanse_serializer(void)
{
  rb_cSerializer = rb_define_class_under(rb_mCleanse, "Serializer", rb_cObject);
  rb_define_singleton_method(rb_cSerializer, "new", rb_cleanse_serializer_new, 1);
  rb_define_method(rb_cSerializer, "to_html", rb_cleanse_serializer_to_html, 0);
}
