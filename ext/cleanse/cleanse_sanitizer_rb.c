#include <ruby/util.h>
#include "cleanse.h"
#include "string_set.h"

VALUE rb_cSanitizer;
VALUE rb_mConfig;
ID rb_cleanse_id_relative;

static VALUE
rb_cleanse_sanitizer_set_flag(VALUE rb_self,
                              VALUE rb_element, VALUE rb_flag, VALUE rb_bool)
{
  CleanseSanitizer *sanitizer;
  Data_Get_Struct(rb_self, CleanseSanitizer, sanitizer);
  Check_Type(rb_flag, T_FIXNUM);
  cleanse_set_element_flags(sanitizer->flags, rb_element,
                            RTEST(rb_bool), FIX2INT(rb_flag));
  return Qnil;
}

static VALUE
rb_cleanse_sanitizer_set_all_flags(VALUE rb_self, VALUE rb_flag, VALUE rb_bool)
{
  long i;
  uint8_t flag;
  CleanseSanitizer *sanitizer;
  Data_Get_Struct(rb_self, CleanseSanitizer, sanitizer);

  Check_Type(rb_flag, T_FIXNUM);
  flag = FIX2INT(rb_flag);

  if (RTEST(rb_bool)) {
    for (i = 0; i < GUMBO_TAG_UNKNOWN; ++i) {
      sanitizer->flags[i] |= flag;
    }
  } else {
    for (i = 0; i < GUMBO_TAG_UNKNOWN; ++i) {
      sanitizer->flags[i] &= ~flag;
    }
  }
  return Qnil;
}

static VALUE
rb_cleanse_sanitizer_allowed_protocols(VALUE rb_self,
                                       VALUE rb_element, VALUE rb_attribute, VALUE rb_allowed)
{
  CleanseSanitizer *sanitizer;
  CleanseElementSanitizer *element_f = NULL;
  CleanseProtocolSanitizer *proto_f = NULL;
  long i;

  Data_Get_Struct(rb_self, CleanseSanitizer, sanitizer);
  element_f = cleanse_sanitizer_get_element(sanitizer,
              cleanse_rb_to_gumbo_tag(rb_element));

  Check_Type(rb_attribute, T_STRING);
  proto_f = cleanse_element_sanitizer_get_proto(element_f,
            StringValuePtr(rb_attribute));

  Check_Type(rb_allowed, T_ARRAY);

  for (i = 0; i < RARRAY_LEN(rb_allowed); ++i) {
    VALUE rb_proto = RARRAY_AREF(rb_allowed, i);
    const char *protocol = NULL;

    if (SYMBOL_P(rb_proto) &&
        SYM2ID(rb_proto) == rb_cleanse_id_relative) {
      string_set_add(&proto_f->allowed, "#");
      protocol = "/";
    } else {
      Check_Type(rb_proto, T_STRING);
      protocol = StringValuePtr(rb_proto);
    }

    string_set_add(&proto_f->allowed, protocol);
  }
  return Qnil;
}

static VALUE
rb_cleanse_sanitizer_set_allow_comments(VALUE rb_self, VALUE rb_bool)
{
  CleanseSanitizer *sanitizer;
  Data_Get_Struct(rb_self, CleanseSanitizer, sanitizer);
  sanitizer->allow_comments = RTEST(rb_bool);
  return rb_bool;
}

static VALUE
rb_cleanse_sanitizer_set_allow_doctype(VALUE rb_self, VALUE rb_bool)
{
  CleanseSanitizer *sanitizer;
  Data_Get_Struct(rb_self, CleanseSanitizer, sanitizer);
  sanitizer->allow_doctype = RTEST(rb_bool);
  return rb_bool;
}

static void
set_in_stringset(string_set_t *set, VALUE rb_attr, bool allow)
{
  strcheck(rb_attr);
  if (allow) {
    string_set_add(set, StringValuePtr(rb_attr));
  } else {
    string_set_remove(set, StringValuePtr(rb_attr));
  }
}

static VALUE
rb_cleanse_sanitizer_allowed_attribute(VALUE rb_self,
                                       VALUE rb_elem, VALUE rb_attr, VALUE rb_allow)
{
  CleanseSanitizer *sanitizer;
  string_set_t *set = NULL;

  Data_Get_Struct(rb_self, CleanseSanitizer, sanitizer);

  if (rb_elem == CSTR2SYM("all")) {
    set = &sanitizer->attr_allowed;
  } else {
    GumboTag tag = cleanse_rb_to_gumbo_tag(rb_elem);
    CleanseElementSanitizer *ef = cleanse_sanitizer_get_element(sanitizer, tag);
    set = &ef->attr_allowed;
  }

  set_in_stringset(set, rb_attr, RTEST(rb_allow));
  return Qnil;
}

static VALUE
rb_cleanse_sanitizer_allowed_class(VALUE rb_self,
                                   VALUE rb_elem, VALUE rb_class, VALUE rb_allow)
{
  CleanseSanitizer *sanitizer;
  string_set_t *set = NULL;

  Data_Get_Struct(rb_self, CleanseSanitizer, sanitizer);

  if (rb_elem == CSTR2SYM("all")) {
    set = &sanitizer->class_allowed;
  } else {
    GumboTag tag = cleanse_rb_to_gumbo_tag(rb_elem);
    CleanseElementSanitizer *ef = cleanse_sanitizer_get_element(sanitizer, tag);
    set = &ef->class_allowed;
  }

  set_in_stringset(set, rb_class, RTEST(rb_allow));
  return Qnil;
}

VALUE
rb_cleanse_sanitizer_new(VALUE klass, VALUE rb_config)
{
  CleanseSanitizer *sanitizer = cleanse_sanitizer_new();
  VALUE rb_sanitizer_obj = Data_Wrap_Struct(klass, NULL, &cleanse_sanitizer_free, sanitizer);

  rb_funcall(rb_sanitizer_obj, rb_intern("setup"), 1, rb_config);

  return rb_sanitizer_obj;
}

void Init_cleanse_sanitizer(void)
{
  rb_cleanse_id_relative = rb_intern("relative");

  rb_cSanitizer = rb_define_class_under(rb_mCleanse, "Sanitizer", rb_cObject);
  rb_mConfig = rb_define_module_under(rb_cSanitizer, "Config");

  rb_define_singleton_method(rb_cSanitizer, "new", rb_cleanse_sanitizer_new, 1);

  rb_define_method(rb_cSanitizer, "set_flag", rb_cleanse_sanitizer_set_flag, 3);
  rb_define_method(rb_cSanitizer, "set_all_flags", rb_cleanse_sanitizer_set_all_flags, 2);

  rb_define_method(rb_cSanitizer, "set_allow_comments", rb_cleanse_sanitizer_set_allow_comments, 1);
  rb_define_method(rb_cSanitizer, "set_allow_doctype", rb_cleanse_sanitizer_set_allow_doctype, 1);

  rb_define_method(rb_cSanitizer, "set_allowed_attribute",
                   rb_cleanse_sanitizer_allowed_attribute, 3);

  rb_define_method(rb_cSanitizer, "set_allowed_class",
                   rb_cleanse_sanitizer_allowed_class, 3);

  rb_define_method(rb_cSanitizer, "set_allowed_protocols",
                   rb_cleanse_sanitizer_allowed_protocols, 3);

  rb_define_const(rb_cSanitizer, "ALLOW",
                  INT2FIX(CLEANSE_SANITIZER_ALLOW));
  rb_define_const(rb_cSanitizer, "REMOVE_CONTENTS",
                  INT2FIX(CLEANSE_SANITIZER_REMOVE_CONTENTS));
  rb_define_const(rb_cSanitizer, "WRAP_WHITESPACE",
                  INT2FIX(CLEANSE_SANITIZER_WRAP_WS));
}
