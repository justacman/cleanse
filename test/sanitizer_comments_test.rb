# frozen_string_literal: true

require "test_helper"

module Cleanse
  class SanitizerCommentsTest < Minitest::Test
    describe "sanitization" do
      context "when :allow_comments is false" do
        def setup
          @sanitizer = Cleanse::Sanitizer.new({ allow_comments: false, elements: ["div"] })
        end

        def test_it_removes_comments
          assert_equal("foo  bar",
                       Cleanse::DocumentFragment.new("foo <!-- comment --> bar", sanitizer: @sanitizer).to_html)
          assert_equal("foo ", Cleanse::DocumentFragment.new("foo <!-- ", sanitizer: @sanitizer).to_html)
          assert_equal("foo ", Cleanse::DocumentFragment.new("foo <!-- - -> bar", sanitizer: @sanitizer).to_html)
          assert_equal("foo bar",
                       Cleanse::DocumentFragment.new("foo <!--\n\n\n\n-->bar", sanitizer: @sanitizer).to_html)
          assert_equal("foo  --&gt; --&gt;bar",
                       Cleanse::DocumentFragment.new("foo <!-- <!-- <!-- --> --> -->bar",
                                                     sanitizer: @sanitizer).to_html)
          assert_equal("foo <div>&gt;bar</div>",
                       Cleanse::DocumentFragment.new("foo <div <!-- comment -->>bar</div>",
                                                     sanitizer: @sanitizer).to_html)

          # Special case: the comment markup is inside a <script>, which makes it
          # text content and not an actual HTML comment.
          assert_equal("",
                       Cleanse::DocumentFragment.new("<script><!-- comment --></script>",
                                                     sanitizer: @sanitizer).to_html)

          sanitizer = Cleanse::Sanitizer.new({ allow_comments: false, elements: ["script"] })
          assert_equal("<script><!-- comment --></script>", Cleanse::DocumentFragment.new("<script><!-- comment --></script>", sanitizer: sanitizer)
                                              .to_html)
        end
      end

      context "when :allow_comments is true" do
        def setup
          @sanitizer = Cleanse::Sanitizer.new({ allow_comments: true, elements: ["div"] })
        end

        def test_it_keeps_comments
          assert_equal("foo <!-- comment --> bar",
                       Cleanse::DocumentFragment.new("foo <!-- comment --> bar", sanitizer: @sanitizer).to_html)
          assert_equal("foo <!-- -->", Cleanse::DocumentFragment.new("foo <!-- ", sanitizer: @sanitizer).to_html)
          assert_equal("foo <!-- - -> bar-->",
                       Cleanse::DocumentFragment.new("foo <!-- - -> bar", sanitizer: @sanitizer).to_html)
          assert_equal("foo <!--\n\n\n\n-->bar",
                       Cleanse::DocumentFragment.new("foo <!--\n\n\n\n-->bar", sanitizer: @sanitizer).to_html)
          assert_equal("foo <!-- <!-- <!-- --> --&gt; --&gt;bar",
                       Cleanse::DocumentFragment.new("foo <!-- <!-- <!-- --> --> -->bar",
                                                     sanitizer: @sanitizer).to_html)
          assert_equal("foo <div>&gt;bar</div>",
                       Cleanse::DocumentFragment.new("foo <div <!-- comment -->>bar</div>",
                                                     sanitizer: @sanitizer).to_html)

          sanitizer = Cleanse::Sanitizer.new({ allow_comments: true, elements: ["script"] })
          assert_equal("<script><!-- comment --></script>", Cleanse::DocumentFragment.new("<script><!-- comment --></script>", sanitizer: sanitizer)
                                              .to_html)
        end
      end
    end
  end
end
