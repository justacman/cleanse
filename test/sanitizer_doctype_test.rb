# frozen_string_literal: true

require "test_helper"

module Cleanse
  class SanitizerDoctypeTest < Minitest::Test
    describe "sanitization" do
      context "when :allow_doctype is false" do
        def setup
          @sanitizer = Cleanse::Sanitizer.new({ allow_doctype: false, elements: ["html"] })
        end

        def test_it_removes_doctype
          assert_equal("<html>foo</html>",
                       Cleanse::Document.new("<!DOCTYPE html><html>foo</html>", sanitizer: @sanitizer).to_html)
          assert_equal("foo", Cleanse::DocumentFragment.new("<!DOCTYPE html>foo", sanitizer: @sanitizer).to_html)
        end

        def test_does_not_allow_doctypes_in_fragments
          assert_equal("foo",
                       Cleanse::DocumentFragment.new("<!DOCTYPE html><html>foo</html>", sanitizer: @sanitizer).to_html)
          assert_equal("foo",
                       Cleanse::DocumentFragment.new('<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01//EN"><html>foo</html>',
                                                     sanitizer: @sanitizer).to_html)
          assert_equal("foo",
                       Cleanse::DocumentFragment.new(
                         '<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"\n    \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\"><html>foo</html>', sanitizer: @sanitizer
                       ).to_html)
        end
      end

      def test_blocks_invalid_doctypes_in_documents
        @sanitizer = Cleanse::Sanitizer.new({ allow_doctype: true, elements: ["html"] })
        assert_equal("<!DOCTYPE html><html>foo</html>",
                     Cleanse::Document.new("<!DOCTYPE blah blah blah><html>foo</html>", sanitizer: @sanitizer).to_html)
        assert_equal("<!DOCTYPE html><html>foo</html>",
                     Cleanse::Document.new("<!DOCTYPE blah><html>foo</html>", sanitizer: @sanitizer).to_html)
        assert_equal("<!DOCTYPE html><html>foo</html>",
                     Cleanse::Document.new('<!DOCTYPE html BLAH "-//W3C//DTD HTML 4.01//EN"><html>foo</html>',
                                           sanitizer: @sanitizer).to_html)
        assert_equal("<!DOCTYPE html><html>foo</html>",
                     Cleanse::Document.new("<!whatever><html>foo</html>", sanitizer: @sanitizer).to_html)
      end

      context "when :allow_doctype is true" do
        def setup
          @sanitizer = Cleanse::Sanitizer.new({ allow_doctype: true, elements: ["html"] })
        end

        def test_it_allows_doctypes_in_documents
          assert_equal("<!DOCTYPE html><html>foo</html>",
                       Cleanse::Document.new("<!DOCTYPE html><html>foo</html>", sanitizer: @sanitizer).to_html)
          assert_equal("<!DOCTYPE html><html>foo</html>",
                       Cleanse::Document.new('<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01//EN"><html>foo</html>',
                                             sanitizer: @sanitizer).to_html)
          assert_equal("<!DOCTYPE html><html>foo</html>",
                       Cleanse::Document.new(
                         '<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"\n    \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\"><html>foo</html>', sanitizer: @sanitizer
                       ).to_html)
        end

        def test_blocks_invalid_doctypes_in_documents
          assert_equal("<!DOCTYPE html><html>foo</html>",
                       Cleanse::Document.new("<!DOCTYPE blah blah blah><html>foo</html>",
                                             sanitizer: @sanitizer).to_html)
          assert_equal("<!DOCTYPE html><html>foo</html>",
                       Cleanse::Document.new("<!DOCTYPE blah><html>foo</html>", sanitizer: @sanitizer).to_html)
          assert_equal("<!DOCTYPE html><html>foo</html>",
                       Cleanse::Document.new('<!DOCTYPE html BLAH "-//W3C//DTD HTML 4.01//EN"><html>foo</html>',
                                             sanitizer: @sanitizer).to_html)
          assert_equal("<!DOCTYPE html><html>foo</html>",
                       Cleanse::Document.new("<!whatever><html>foo</html>", sanitizer: @sanitizer).to_html)
        end

        def test_blocks_doctypes_in_fragments
          assert_equal("foo",
                       Cleanse::DocumentFragment.new("<!DOCTYPE html><html>foo</html>", sanitizer: @sanitizer).to_html)
          assert_equal("foo",
                       Cleanse::DocumentFragment.new('<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01//EN"><html>foo</html>',
                                                     sanitizer: @sanitizer).to_html)
          assert_equal("foo",
                       Cleanse::DocumentFragment.new(
                         '<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"\n    \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\"><html>foo</html>', sanitizer: @sanitizer
                       ).to_html)
        end
      end
    end
  end
end
