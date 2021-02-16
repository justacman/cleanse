#include "cleanse.h"

#include "gumbo.h"
#include "parser.h"
#include "vector.h"

// TODO
static GumboNode* create_node(GumboNodeType type)
{
  GumboNode* node = gumbo_alloc(sizeof(GumboNode));
  node->parent = NULL;
  node->index_within_parent = -1;
  node->type = type;
  node->parse_flags = GUMBO_INSERTION_NORMAL;
  return node;
}

// A node to separate an element that has been sanitized out
static GumboNode *create_whitespace_node()
{
  GumboNode *node = create_node(GUMBO_NODE_WHITESPACE);
  node->parse_flags = GUMBO_INSERTION_BY_PARSER;
  node->v.text.text = strdup(" ");
  node->v.text.start_pos = kGumboEmptySourcePosition;
  return node;
}

static void readjust_children(unsigned int pos, GumboVector *children, GumboNode *parent)
{
  unsigned int x;
  for (x = pos; x < children->length; ++x) {
    GumboNode *child = children->data[x];
    if (parent) {
      child->parent = parent;
    }
    child->index_within_parent = x;
  }
}

void
cleanse_remove_child_at(GumboNode *parent, unsigned int pos, bool wrap)
{
  GumboVector *children = &parent->v.element.children;

  if (wrap) {
    GumboNode *whitespace_node = create_whitespace_node();
    whitespace_node->parent = parent;
    whitespace_node->index_within_parent = pos;
    children->data[pos] = whitespace_node;
  } else {
    gumbo_vector_remove_at(pos, children);
  }

  readjust_children(pos, children, NULL);
}

void
cleanse_reparent_children_at(GumboNode *parent, GumboVector *new_children,
                             unsigned int pos, bool wrap)
{
  GumboVector *children = &parent->v.element.children;
  void **new_children_ary;
  unsigned int new_children_len;

  if (!new_children->length) {
    cleanse_remove_child_at(parent, pos, wrap);
    return;
  }

  if (wrap) {
    new_children_len = new_children->length + 2;
    new_children_ary = alloca(new_children_len * sizeof(void*));
    memcpy(new_children_ary + 1,
           new_children->data,
           new_children->length * sizeof(void *));
    new_children_ary[0] = create_whitespace_node();
    new_children_ary[new_children_len - 1] = create_whitespace_node();
  } else {
    new_children_len = new_children->length;
    new_children_ary = new_children->data;
  }

  gumbo_vector_splice(
    pos, 1, new_children_ary, new_children_len,
    children);

  readjust_children(pos, children, parent);

  new_children->length = 0;
}

