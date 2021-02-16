#include <ruby/util.h>
#include <ctype.h>

#include "cleanse.h"
#include "attribute.h"
#include "util.h"
#include "string_buffer.h"
#include "vector.h"
#include "string_set.h"

typedef struct {
  st_table *tags_visited;
} context;

static void
sanitize_node(const CleanseSanitizer *sanitizer, context *ctx, GumboNode *node);

static int
free_each_element_sanitizer(st_data_t _unused1, st_data_t _ef, st_data_t _unused2)
{
  CleanseElementSanitizer *ef = (CleanseElementSanitizer *)_ef;
  CleanseProtocolSanitizer *proto;
  (void)_unused1;
  (void)_unused2;

  proto = ef->protocols;
  while (proto) {
    CleanseProtocolSanitizer *next = proto->next;
    xfree(proto->name);
    string_set_free(&proto->allowed);
    xfree(proto);
    proto = next;
  }

  string_set_free(&ef->attr_allowed);
  string_set_free(&ef->attr_required);
  string_set_free(&ef->class_allowed);
  xfree(ef);

  return ST_CONTINUE;
}

void
cleanse_sanitizer_free(void *_sanitizer)
{
  CleanseSanitizer *sanitizer = _sanitizer;

  string_set_free(&sanitizer->attr_allowed);
  string_set_free(&sanitizer->class_allowed);

  st_foreach(sanitizer->element_sanitizers, &free_each_element_sanitizer, 0);
  st_free_table(sanitizer->element_sanitizers);

  xfree(sanitizer);
}

CleanseSanitizer *
cleanse_sanitizer_new(void)
{
  CleanseSanitizer *sanitizer = xcalloc(1, sizeof(CleanseSanitizer));

  string_set_new(&sanitizer->attr_allowed);
  string_set_new(&sanitizer->class_allowed);
  sanitizer->element_sanitizers = st_init_numtable();

  return sanitizer;
}

static CleanseElementSanitizer *
try_find_element(const CleanseSanitizer *sanitizer, GumboTag tag)
{
  st_data_t data;

  if (st_lookup(sanitizer->element_sanitizers, (st_data_t)tag, (st_data_t *)&data)) {
    return (CleanseElementSanitizer *)data;
  }

  return NULL;
}

CleanseElementSanitizer *
cleanse_sanitizer_get_element(CleanseSanitizer *sanitizer, GumboTag t)
{
  CleanseElementSanitizer *ef = try_find_element(sanitizer, t);

  if (ef == NULL) {
    ef = xmalloc(sizeof(CleanseElementSanitizer));
    ef->max_nested = 0;
    string_set_new(&ef->attr_allowed);
    string_set_new(&ef->attr_required);
    string_set_new(&ef->class_allowed);
    ef->protocols = NULL;
    st_insert(sanitizer->element_sanitizers, (st_data_t)t, (st_data_t)ef);
  }

  return ef;
}

CleanseProtocolSanitizer *cleanse_element_sanitizer_get_proto(
  CleanseElementSanitizer *element, const char *protocol_name)
{
  CleanseProtocolSanitizer *proto = element->protocols;

  while (proto) {
    if (!strcmp(proto->name, protocol_name)) {
      return proto;
    }

    proto = proto->next;
  }

  proto = xmalloc(sizeof(CleanseProtocolSanitizer));
  proto->name = ruby_strdup(protocol_name);
  string_set_new(&proto->allowed);
  proto->next = element->protocols;

  element->protocols = proto;
  return proto;
}

static bool
sanitize_attributes(const CleanseSanitizer *sanitizer, GumboElement *element);

static void
remove_child(GumboNode *parent, GumboNode *child, unsigned int pos, uint8_t flags)
{
  bool wrap_whitespace = (flags & CLEANSE_SANITIZER_WRAP_WS);

  if ((flags & CLEANSE_SANITIZER_REMOVE_CONTENTS)) {
    cleanse_remove_child_at(parent, pos, wrap_whitespace);
  } else {
    cleanse_reparent_children_at(parent,
                                 &child->v.element.children, pos, wrap_whitespace);
  }

  gumbo_destroy_node(child);
}

static bool
try_remove_child(
  const CleanseSanitizer *sanitizer, GumboNode *parent, GumboNode *child, unsigned int pos)
{
  if (child->type == GUMBO_NODE_ELEMENT || child->type == GUMBO_NODE_TEMPLATE) {
    GumboTag tag = child->v.element.tag;
    bool should_remove = false;
    uint8_t flags =
      (tag == GUMBO_TAG_UNKNOWN) ? 0
      : sanitizer->flags[tag];

    if ((flags & CLEANSE_SANITIZER_ALLOW) == 0) {
      should_remove = true;
    }

    if (!should_remove) {
      // anything in <iframe> must be removed, if it's kept
      if (tag == GUMBO_TAG_IFRAME && child->v.element.children.length > 0) {
        cleanse_remove_child_at(child, 0, sanitizer->flags[tag]);
      }
      if (!sanitize_attributes(sanitizer, &child->v.element)) {
        should_remove = true;
      }
    }

    if (should_remove) {
      // the contents of these are considered "text node" and must be removed
      if ((tag == GUMBO_TAG_SCRIPT || tag == GUMBO_TAG_STYLE || tag == GUMBO_TAG_MATH || tag == GUMBO_TAG_SVG) && child->v.element.children.length > 0) {
        cleanse_remove_child_at(child, 0, sanitizer->flags[tag]);
      }

      remove_child(parent, child, pos, flags);
      return true;
    }
  } else if (child->type == GUMBO_NODE_COMMENT && !sanitizer->allow_comments) {
    cleanse_remove_child_at(parent, pos, false);
    gumbo_destroy_node(child);
    return true;
  }

  return false;
}

static void
sanitize_children(const CleanseSanitizer *sanitizer, context *ctx, GumboNode *parent, GumboVector *children)
{
  unsigned int x;

  for (x = 0; x < children->length; ++x) {
    GumboNode *child = children->data[x];
    GumboTag tag = GUMBO_TAG_LAST;
    st_data_t tag_key = GUMBO_TAG_LAST, n = 0;

    if (child->type == GUMBO_NODE_ELEMENT || child->type == GUMBO_NODE_TEMPLATE) {
      tag = child->v.element.tag;
      tag_key = (st_data_t)tag;

      CleanseElementSanitizer *ef = try_find_element(sanitizer, tag);
      if (ef && ef->max_nested > 0 &&
          st_lookup(ctx->tags_visited, tag_key, (st_data_t *)&n) &&
          n >= ef->max_nested) {
        remove_child(parent, child, x, (tag == GUMBO_TAG_UNKNOWN) ? 0 : sanitizer->flags[tag]);
        x--;
        continue;
      }
    }

    if (try_remove_child(sanitizer, parent, child, x)) {
      x--;
      continue;
    }

    if (tag_key != GUMBO_TAG_LAST) {
      n += 1;
      st_insert(ctx->tags_visited, tag_key, (st_data_t)n);
    }

    sanitize_node(sanitizer, ctx, child);

    if (tag_key != GUMBO_TAG_LAST) {
      n -= 1;
      if (n == 0) {
        st_delete(ctx->tags_visited, &tag_key, NULL);
      } else {
        st_insert(ctx->tags_visited, tag_key, (st_data_t)n);
      }
    }
  }
}

static bool
has_allowed_protocol(const string_set_t *protocols_allowed, GumboAttribute *attr)
{
  const char *value = attr->value;
  char *proto;
  long i, len = 0;

  // Trim leading space
  while (isspace(*value)) {
    value++;
  }

  while (value[len] && value[len] != '/' && value[len] != ':' && value[len] != '#') {
    len++;
  }

  attr->value = strdup(value);

  if (value[len] == '\0' || value[len] == '/') {
    return string_set_contains(protocols_allowed, "/");
  }

  if (value[len] == '\0' || value[len] == '#') {
    return string_set_contains(protocols_allowed, "#");
  }

  // Make the protocol name case insensitive
  proto = alloca(len + 1);
  for (i = 0; i < len; ++i) {
    proto[i] = gumbo_tolower(value[i]);
  }
  proto[len] = 0;

  return string_set_contains(protocols_allowed, proto);
}

static bool
sanitize_class_attribute(const CleanseSanitizer *sanitizer,
                         const CleanseElementSanitizer *element_f, GumboAttribute *attr)
{
  const string_set_t *allowed_global = NULL;
  const string_set_t *allowed_local = NULL;
  GumboStringBuffer buf;
  int valid_classes = 0;
  char *value, *end;

  if (sanitizer->class_allowed.size) {
    allowed_global = &sanitizer->class_allowed;
  }

  if (element_f && element_f->class_allowed.size) {
    allowed_local = &element_f->class_allowed;
  }

  // everything goes through
  if (!allowed_global && !allowed_local) {
    return true;
  }

  value = (char *)attr->value;
  end = value + strlen(value);

  while (value < end) {
    while (value < end && isspace(*value)) {
      value++;
    }

    if (value < end) {
      char *class = value;
      bool allowed = false;
      while (value < end && !isspace(*value)) {
        value++;
      }

      *value = 0;

      if (allowed_local &&
          string_set_contains(allowed_local, class)) {
        allowed = true;
      }

      if (allowed_global &&
          string_set_contains(allowed_global, class)) {
        allowed = true;
      }

      if (allowed) {
        if (!valid_classes) {
          strbuf_init(&buf);
        } else {
          strbuf_puts(&buf, " ");
        }

        strbuf_puts(&buf, class);
        valid_classes++;
      }

      value = value + 1;
    }
  }

  /* If we've found classes that passed the allow list,
   * we need to set the new value in the attribute */
  if (valid_classes) {
    strbuf_put(&buf, "\0", 1);
    gumbo_attribute_set_value(attr, buf.data);
    strbuf_free(&buf);
    return true;
  }

  return false;
}

static bool
should_keep_attribute(const CleanseSanitizer *sanitizer,
                      const CleanseElementSanitizer *element_f, GumboAttribute *attr)
{
  bool allowed = false;

  if (element_f && string_set_contains(&element_f->attr_allowed, attr->name)) {
    allowed = true;
  }

  if (!allowed && string_set_contains(&sanitizer->attr_allowed, attr->name)) {
    allowed = true;
  }

  if (!allowed) {
    return false;
  }

  if (element_f && element_f->protocols) {
    CleanseProtocolSanitizer *proto = element_f->protocols;
    while (proto) {
      if (!strcmp(attr->name, proto->name)) {
        if (!has_allowed_protocol(&proto->allowed, attr)) {
          return false;
        }
        break;
      }
      proto = proto->next;
    }
  }

  if (!strcmp(attr->name, "class")) {
    if (!sanitize_class_attribute(sanitizer, element_f, attr)) {
      return false;
    }
  }

  return true;
}

static bool
sanitize_attributes(const CleanseSanitizer *sanitizer, GumboElement *element)
{
  GumboVector *attributes = &element->attributes;
  CleanseElementSanitizer *element_f = try_find_element(sanitizer, element->tag);
  unsigned int x;

  for (x = 0; x < attributes->length; ++x) {
    GumboAttribute *attr = attributes->data[x];

    if (!should_keep_attribute(sanitizer, element_f, attr)) {
      gumbo_element_remove_attribute_at(element, x);
      x--;
      continue;
    } else {
      // Prevent the use of `<meta>` elements that set a charset other than UTF-8,
      // since output is always UTF-8.
      if (element->tag == GUMBO_TAG_META) {
        if (!strcmp(attr->name, "charset") && strcmp(attr->value, "utf-8")) {
          gumbo_attribute_set_value(attr, "utf-8");
        }
      }
    }
  }

  if (element_f && element_f->attr_required.size) {
    string_set_t *required = &element_f->attr_required;

    if (string_set_contains(required, "*")) {
      return attributes->length > 0;
    }

    for (x = 0; x < attributes->length; ++x) {
      GumboAttribute *attr = attributes->data[x];
      if (string_set_contains(required, attr->name)) {
        return true;
      }
    }

    return false;
  }

  return true;
}

static void
sanitize_node(const CleanseSanitizer *sanitizer, context *ctx, GumboNode *node)
{
  switch (node->type) {
  case GUMBO_NODE_DOCUMENT:
    sanitize_children(sanitizer, ctx, node, &node->v.document.children);
    break;

  case GUMBO_NODE_ELEMENT:
  case GUMBO_NODE_TEMPLATE:
    sanitize_children(sanitizer, ctx, node, &node->v.element.children);
    break;

  // Nothing else to sanitize
  case GUMBO_NODE_TEXT:
  case GUMBO_NODE_WHITESPACE:
  case GUMBO_NODE_CDATA:
  case GUMBO_NODE_COMMENT:
    break;
  }
}

void
cleanse_node_sanitize(const CleanseSanitizer *sanitizer, GumboNode *node)
{
  context ctx;
  ctx.tags_visited = st_init_numtable();
  sanitize_node(sanitizer, &ctx, node);
  st_free_table(ctx.tags_visited);
}
