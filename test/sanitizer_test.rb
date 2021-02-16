# frozen_string_literal: true

require "test_helper"

module Cleanse
  class SanitizerTest < Minitest::Test
    def test_it_sanitizes_by_default
      html = "<a href='https://google.com'>wow!</a>"
      doc = Cleanse::DocumentFragment.new(html)
      assert_equal "wow!", doc.to_html
    end

    def test_it_can_get_elements
      hash = {
        elements: %w[
          a
        ]
      }
      sanitizer = Cleanse::Sanitizer.new(hash)
      assert_equal %w[a], sanitizer.elements
    end

    def test_it_can_keep_attributes
      hash = {
        elements: %w[
          a
        ],

        attributes: {
          "a" => %w[href]
        },

        protocols: {
          "a" => { "href" => ["ftp", "http", "https", "mailto", :relative] }
        }
      }
      sanitizer = Cleanse::Sanitizer.new(hash)
      html = "<a href='https://google.com'>wow!</a>"
      doc = Cleanse::DocumentFragment.new(html, sanitizer: sanitizer)
      assert_equal '<a href="https://google.com">wow!</a>', doc.to_html
    end

    def test_it_can_be_turned_off
      html = "<a href='https://google.com'>wow!</a>"
      doc = Cleanse::DocumentFragment.new(html, sanitizer: nil)
      assert_equal '<a href="https://google.com">wow!</a>', doc.to_html
    end

    describe "#document" do
      def setup
        @sanitizer = Cleanse::Sanitizer.new(elements: ["html"])
      end

      def test_should_sanitize_an_HTML_document
        assert_equal("<!DOCTYPE html><html>Lorem ipsum dolor sitamet </html>",
                     Cleanse::Document.new(
                       '<!doctype html><html><b>Lo<!-- comment -->rem</b> <a href="pants" title="foo">ipsum</a> <a href="http://foo.com/"><strong>dolor</strong></a> sit<br/>amet <script>alert("hello world");</script></html>', sanitizer: @sanitizer
                     ).to_html)
      end

      def test_should_not_modify_the_input_string
        input = "<!DOCTYPE html><b>foo</b>"
        Cleanse::Document.new("<!DOCTYPE html><b>foo</b>", sanitizer: @sanitizer).to_html
        assert_equal(input, "<!DOCTYPE html><b>foo</b>")
      end

      def test_should_not_choke_on_frozen_documents
        assert_equal("<!DOCTYPE html><html>foo</html>",
                     Cleanse::Document.new("<!doctype html><html><b>foo</b>", sanitizer: @sanitizer).to_html)
      end

      def test_should_normalize_newlines
        assert_equal("<!DOCTYPE html><html>a\n\n\n\n\nz</html>",
                     Cleanse::Document.new("a\r\n\n\r\r\r\nz", sanitizer: @sanitizer).to_html)
      end

      def test_should_strip_control_characters_except_ASCII_whitespace
        sample_control_chars = "\u0001\u0008\u000b\u000e\u001f\u007f\u009f"
        whitespace = "\t\n\f\u0020"
        assert_equal("<!DOCTYPE html><html>a#{whitespace}z</html>",
                     Cleanse::Document.new("a#{sample_control_chars}#{whitespace}z", sanitizer: @sanitizer).to_html)
      end

      def test_should_strip_non_characters
        sample_non_chars = "\ufdd0\ufdef\ufffe\uffff\u{1fffe}\u{1ffff}\u{2fffe}\u{2ffff}\u{3fffe}\u{3ffff}\u{4fffe}\u{4ffff}\u{5fffe}\u{5ffff}\u{6fffe}\u{6ffff}\u{7fffe}\u{7ffff}\u{8fffe}\u{8ffff}\u{9fffe}\u{9ffff}\u{afffe}\u{affff}\u{bfffe}\u{bffff}\u{cfffe}\u{cffff}\u{dfffe}\u{dffff}\u{efffe}\u{effff}\u{ffffe}\u{fffff}\u{10fffe}\u{10ffff}"
        assert_equal("<!DOCTYPE html><html>az</html>",
                     Cleanse::Document.new("a#{sample_non_chars}z", sanitizer: @sanitizer).to_html)
      end

      describe "when html body exceeds Nokogumbo::DEFAULT_MAX_TREE_DEPTH" do
        def setup
          html = nest_html_content("<b>foo</b>", Nokogumbo::DEFAULT_MAX_TREE_DEPTH)
          @content = "<html>#{html}</html>"
        end

        def test_raises_an_RuntimeError_exception
          assert_raises RuntimeError do
            Cleanse::Document.new(@content)
          end
        end

        describe "and :max_tree_depth of -1 is supplied in :parser_options" do
          def setup
            @sanitizer = Sanitizer.new(elements: ["html"], parser_options: { max_tree_depth: -1 })
            html = nest_html_content("<b>foo</b>", Nokogumbo::DEFAULT_MAX_TREE_DEPTH)
            @content = "<html>#{html}</html>"
          end

          def test_does_not_raise_an_RuntimeError_exception
            skip "skip this for now"
            assert_equal("<html>foo</html>", Cleanse::Document.new(@content, sanitizer: @sanitizer).to_html)
          end
        end
      end
    end

    describe "#fragment" do
      def setup
        @sanitizer = Cleanse::Sanitizer.new(elements: ["html"])
      end

      def test_should_sanitize_an_HTML_fragment
        assert_equal("Lorem ipsum dolor sitamet ",
                     Cleanse::DocumentFragment.new(
                       '<b>Lo<!-- comment -->rem</b> <a href="pants" title="foo">ipsum</a> <a href="http://foo.com/"><strong>dolor</strong></a> sit<br/>amet <script>alert("hello world");</script>', sanitizer: @sanitizer
                     ).to_html)
      end

      def test_should_not_modify_the_input_string
        input = "<b>foo</b>"
        Cleanse::DocumentFragment.new("<b>foo</b>", sanitizer: @sanitizer).to_html
        assert_equal(input, "<b>foo</b>")
      end

      def test_should_not_choke_on_fragments_containing_html_or_body
        assert_equal("foo", Cleanse::DocumentFragment.new("<html><b>foo</b></html>").to_html)
        assert_equal("foo", Cleanse::DocumentFragment.new("<body><b>foo</b></body>").to_html)
        assert_equal("foo", Cleanse::DocumentFragment.new("<html><body><b>foo</b></body></html>").to_html)
        assert_equal("foo",
                     Cleanse::DocumentFragment.new("<!DOCTYPE html><html><body><b>foo</b></body></html>").to_html)
      end

      def test_should_not_choke_on_frozen_fragments
        assert_equal("foo", Cleanse::DocumentFragment.new("<b>foo</b>", sanitizer: @sanitizer).to_html)
      end

      def test_should_normalize_newlines
        assert_equal("a\n\n\n\n\nz",
                     Cleanse::DocumentFragment.new("a\r\n\n\r\r\r\nz", sanitizer: @sanitizer).to_html)
      end

      def test_should_strip_control_characters_except_ASCII_whitespace
        sample_control_chars = "\u0001\u0008\u000b\u000e\u001f\u007f\u009f"
        whitespace = "\t\n\f\u0020"
        assert_equal("a#{whitespace}z",
                     Cleanse::DocumentFragment.new("a#{sample_control_chars}#{whitespace}z",
                                                   sanitizer: @sanitizer).to_html)
      end

      def test_should_strip_non_characters
        sample_non_chars = "\ufdd0\ufdef\ufffe\uffff\u{1fffe}\u{1ffff}\u{2fffe}\u{2ffff}\u{3fffe}\u{3ffff}\u{4fffe}\u{4ffff}\u{5fffe}\u{5ffff}\u{6fffe}\u{6ffff}\u{7fffe}\u{7ffff}\u{8fffe}\u{8ffff}\u{9fffe}\u{9ffff}\u{afffe}\u{affff}\u{bfffe}\u{bffff}\u{cfffe}\u{cffff}\u{dfffe}\u{dffff}\u{efffe}\u{effff}\u{ffffe}\u{fffff}\u{10fffe}\u{10ffff}"
        assert_equal("az", Cleanse::DocumentFragment.new("a#{sample_non_chars}z", sanitizer: @sanitizer).to_html)
      end

      describe "when html body exceeds Nokogumbo::DEFAULT_MAX_TREE_DEPTH" do
        def setup
          html = nest_html_content("<b>foo</b>", Nokogumbo::DEFAULT_MAX_TREE_DEPTH)
          @content = "<body>#{html}</body>"
          @sanitizer = Cleanse::Sanitizer.new(parser_options: { max_tree_depth: -1 })
        end

        def test_raises_an_RuntimeError_exception
          assert_raises RuntimeError do
            Cleanse::DocumentFragment.new(@content)
          end
        end

        describe "and :max_tree_depth of -1 is supplied in :parser_options" do
          def test_does_not_raise_an_RuntimeError_exception
            skip "skip this for now"
            assert_equal("foo", Cleanse::DocumentFragment.new(@content, sanitizer: @sanitizer).to_html)
          end
        end
      end
    end
  end
end
