#include "string_buffer.h"
#include "attribute.h"
#include "cleanse.h"
#include "gumbo.h"

VALUE rb_cDocument;
VALUE rb_cDocumentFragment;
VALUE rb_cNode;
VALUE rb_cTextNode;
VALUE rb_cCommentNode;
VALUE rb_cElementNode;

ID g_id_sanitizer;
ID g_id_html;

VALUE cleanse_node_alloc(VALUE klass, VALUE rb_document, GumboNode *node)
{
  VALUE rb_node = Data_Wrap_Struct(klass, NULL, NULL, node);
  rb_iv_set(rb_node, "@document", rb_document);
  return rb_node;
}

static void cleanse_free_output(GumboOutput *output)
{
  gumbo_destroy_output(output);
}

// Thank you, Internet https://git.io/JtPnH
VALUE preprocess(VALUE rb_text)
{
  char *input = RSTRING_PTR(rb_text);
  long i, input_len = RSTRING_LEN(rb_text), output_len = 0;
  char *output = ALLOC_N(char, input_len);
  VALUE result;

  if (!output) {
    return Qnil;
  }

  for(i = 0; i < input_len; ++i) {
    long remain = input_len - i;

    /* ASCII, minus DEL */
    if (input[i] == '\t' ||
        input[i] == '\r' ||
        input[i] == '\n' ||
        input[i] == '\f' ||
        (input[i] >= ' ' && input[i] < 127)) {
      output[output_len++] = input[i];
    }
    /* 2-byte sequence */
    else if (remain >= 2 &&
             (input[i] & 0xe0) == 0xc0 &&
             (input[i + 1] & 0xc0) == 0x80) {
      i+=1;
    }
    /* 3-byte sequence */
    else if (remain >= 3 &&
             (input[i] & 0xf0) == 0xe0 &&
             (input[i + 1] & 0xc0) == 0x80 &&
             (input[i + 2] & 0xc0) == 0x80) {
      i+=2;
    }
    /* 4-byte sequence */
    else if (remain >= 4 &&
             (input[i] & 0xf8) == 0xf0 &&
             (input[i + 1] & 0xc0) == 0x80 &&
             (input[i + 2] & 0xc0) == 0x80 &&
             (input[i + 3] & 0xc0) == 0x80) {
      i+=3;
    }
  }

  result = rb_str_new(output, output_len);
  free(output);
  return result;
}

GumboOutput *cleanse_parse_fragment(VALUE rb_text, GumboTag fragment_ctx)
{
  GumboOptions options = kGumboDefaultOptions;
  options.max_errors = 10;
  if (fragment_ctx != GUMBO_TAG_LAST) {
    options.fragment_context = gumbo_normalized_tagname(fragment_ctx);
  }

  VALUE rb_clean = preprocess(rb_text);

  return gumbo_parse_with_options(&options, RSTRING_PTR(rb_clean), RSTRING_LEN(rb_clean));
}

VALUE cleanse_parse_to_rb(VALUE klass, VALUE rb_text, GumboTag fragment_ctx)
{
  GumboOutput *output = cleanse_parse_fragment(rb_text, fragment_ctx);
  if (output->status != GUMBO_STATUS_OK) {
    rb_raise(rb_eRuntimeError, "could not parse rb_text");
  }
  return Data_Wrap_Struct(klass, NULL, &cleanse_free_output, output);
}

static VALUE
rb_cleanse_parse_and_sanitize(int argc, VALUE *argv, VALUE klass, GumboTag fragment_ctx)
{
  VALUE rb_text, rb_sanitizer, rb_sanitizer_config, rb_fragment, rb_opts;

  rb_scan_args(argc, argv, "1:", &rb_text, &rb_opts);

  if (!NIL_P(rb_opts)) {
    rb_sanitizer = rb_hash_lookup(rb_opts, CSTR2SYM("sanitizer"));
  } else { // sanitize by default!
    rb_sanitizer_config = rb_const_get_at(rb_mConfig, rb_intern("DEFAULT"));
    rb_sanitizer = rb_funcall(rb_cSanitizer, rb_intern("new"), 1, rb_sanitizer_config);
  }

  strcheck(rb_text);

  rb_fragment = cleanse_parse_to_rb(klass, rb_text, fragment_ctx);

  rb_ivar_set(rb_fragment, g_id_sanitizer, rb_sanitizer);
  if (!NIL_P(rb_sanitizer)) {
    GumboOutput *output = NULL;
    CleanseSanitizer *sanitizer = NULL;

    if (!rb_obj_is_kind_of(rb_sanitizer, rb_cSanitizer)) {
      rb_raise(rb_eTypeError, "expected a Cleanse::Sanitizer instance");
    }

    Data_Get_Struct(rb_fragment, GumboOutput, output);
    Data_Get_Struct(rb_sanitizer, CleanseSanitizer, sanitizer);

    if (fragment_ctx == GUMBO_TAG_LAST) {
      cleanse_node_sanitize(sanitizer, output->document);
    } else {
      cleanse_node_sanitize(sanitizer, output->root);
    }
  }

  return rb_fragment;
}

static VALUE
rb_cleanse_doc_parse(int argc, VALUE *argv, VALUE klass)
{
  return rb_cleanse_parse_and_sanitize(argc, argv, klass, GUMBO_TAG_HTML);
}

static VALUE
rb_cleanse_doc_fragment_parse(int argc, VALUE *argv, VALUE klass)
{
  return rb_cleanse_parse_and_sanitize(argc, argv, klass, GUMBO_TAG_DIV);
}

void Init_cleanse_document(void)
{
  g_id_sanitizer = rb_intern("@sanitizer");
  g_id_html = rb_intern("html");

  rb_cDocument = rb_define_class_under(rb_mCleanse, "Document", rb_cObject);
  rb_define_singleton_method(rb_cDocument, "new", rb_cleanse_doc_parse, -1);

  rb_cDocumentFragment = rb_define_class_under(rb_mCleanse, "DocumentFragment", rb_cObject);
  rb_define_singleton_method(rb_cDocumentFragment, "new", rb_cleanse_doc_fragment_parse, -1);

  rb_cNode = rb_define_class_under(rb_mCleanse, "Node", rb_cObject);
  rb_cTextNode = rb_define_class_under(rb_mCleanse, "TextNode", rb_cNode);
  rb_cCommentNode = rb_define_class_under(rb_mCleanse, "CommentNode", rb_cNode);
  rb_cElementNode = rb_define_class_under(rb_mCleanse, "ElementNode", rb_cNode);
}
