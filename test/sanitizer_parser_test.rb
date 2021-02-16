# frozen_string_literal: true

require "test_helper"

module Cleanse
  class SanitizerParserTest < Minitest::Test
    def test_should_translate_valid_entities_into_characters
      assert_equal "'é&amp;", Cleanse::DocumentFragment.new("&apos;&eacute;&amp;").to_html
    end

    def test_should_translate_orphaned_ampersands_into_entities
      assert_equal "at&amp;t", Cleanse::DocumentFragment.new("at&t").to_html
    end

    def test_should_not_add_newlines_after_tags_when_serializing_a_fragment
      sanitizer = Cleanse::Sanitizer.new({
                                           elements: %w[div p]
                                         })

      assert_equal("<div>foo\n\n<p>bar</p><div>\nbaz</div></div><div>quux</div>",
                   Cleanse::DocumentFragment.new("<div>foo\n\n<p>bar</p><div>\nbaz</div></div><div>quux</div>",
                                                 sanitizer: sanitizer).to_html)
    end

    def test_should_not_have_the_Nokogiri_1_4_2_unterminated_script_style_element_bug
      assert_equal("foo ", Cleanse::DocumentFragment.new("foo <script>bar").to_html)

      assert_equal("foo ", Cleanse::DocumentFragment.new("foo <style>bar").to_html)
    end

    def test_ambiguous_non_tag_brackets_like__should_be_parsed_correctly
      assert_equal("1 &gt; 2 and 2 &lt; 1", Cleanse::DocumentFragment.new("1 > 2 and 2 < 1").to_html)

      assert_equal("OMG HAPPY BIRTHDAY! *&lt;:-D", Cleanse::DocumentFragment.new("OMG HAPPY BIRTHDAY! *<:-D").to_html)
    end
  end
end
