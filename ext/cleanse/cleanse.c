#include "cleanse.h"

#include "gumbo.h"

VALUE rb_mCleanse;

GumboTag cleanse_rb_to_gumbo_tag(VALUE rb_tag)
{
  const char *tag_name;
  GumboTag tag;

  if (SYMBOL_P(rb_tag)) {
    tag_name = rb_id2name(SYM2ID(rb_tag));
  } else {
    tag_name = StringValuePtr(rb_tag);
  }

  tag = gumbo_tagn_enum(tag_name, strlen(tag_name));

  if (tag == GUMBO_TAG_UNKNOWN) {
    rb_raise(rb_eArgError, "unknown tag: '%s'", tag_name);
  }

  return tag;
}

void
cleanse_set_element_flags(uint8_t *flags, VALUE rb_element, bool set, int flag)
{
  GumboTag t = cleanse_rb_to_gumbo_tag(rb_element);
  int f = (int)t;

  if (set) {
    flags[f] |= flag;
  }
  else {
    flags[f] &= ~flag;
  }
}

void strcheck(VALUE rb_str)
{
  Check_Type(rb_str, T_STRING);
  if (ENCODING_GET_INLINED(rb_str) != rb_utf8_encindex()) {
    rb_raise(rb_eEncodingError, "Expected UTF-8 encoding");
  }
}

__attribute__ ((visibility ("default"))) void Init_cleanse(void)
{
  rb_mCleanse = rb_define_module("Cleanse");
  Init_cleanse_document();
  Init_cleanse_sanitizer();
  Init_cleanse_serializer();
}
