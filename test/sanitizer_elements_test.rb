# frozen_string_literal: true

require "test_helper"

module Cleanse
  class SanitizerTest < Minitest::Test
    describe "sanitize" do
      context "Default config" do
        def test_remove_non_allowlisted_elements_leaving_safe_contents_behind
          assert_equal("foo bar baz quux",
                       Cleanse::DocumentFragment.new('foo <b>bar</b> <strong><a href="#a">baz</a></strong> quux').to_html)
          assert_equal("", Cleanse::DocumentFragment.new('<script>alert("<xss>");</script>').to_html)
          assert_equal("&lt;", Cleanse::DocumentFragment.new('<<script>script>alert("<xss>");</<script>>').to_html)
          assert_equal('&lt; script &lt;&gt;&gt; alert("");',
                       Cleanse::DocumentFragment.new('< script <>> alert("<xss>");</script>').to_html)
        end

        def test_should_surround_the_contents_of_whitespace_elements_with_space_characters_when_removing_the_element
          assert_equal("foo bar baz", Cleanse::DocumentFragment.new("foo<div>bar</div>baz").to_html)
          assert_equal("foo bar baz", Cleanse::DocumentFragment.new("foo<br>bar<br>baz").to_html)
          assert_equal("foo bar baz", Cleanse::DocumentFragment.new("foo<hr>bar<hr>baz").to_html)
        end

        def test_should_not_choke_on_several_instances_of_the_same_element_in_a_row
          assert_equal("",
                       Cleanse::DocumentFragment.new('<img src="http://www.google.com/intl/en_ALL/images/logo.gif"><img src="http://www.google.com/intl/en_ALL/images/logo.gif"><img src="http://www.google.com/intl/en_ALL/images/logo.gif"><img src="http://www.google.com/intl/en_ALL/images/logo.gif">').to_html)
        end

        def test_should_not_preserve_the_content_of_removed_iframe_elements
          assert_equal("", Cleanse::DocumentFragment.new("<iframe>hello! <script>alert(0)</script></iframe>").to_html)
        end

        def test_should_not_preserve_the_content_of_removed_math_elements
          assert_equal("", Cleanse::DocumentFragment.new("<math>hello! <script>alert(0)</script></math>").to_html)
        end

        def test_should_not_preserve_the_content_of_removed_noembed_elements
          assert_equal("", Cleanse::DocumentFragment.new("<noembed>hello! <script>alert(0)</script></noembed>").to_html)
        end

        def test_should_not_preserve_the_content_of_removed_noframes_elements
          assert_equal("",
                       Cleanse::DocumentFragment.new("<noframes>hello! <script>alert(0)</script></noframes>").to_html)
        end

        def test_should_not_preserve_the_content_of_removed_noscript_elements
          assert_equal("",
                       Cleanse::DocumentFragment.new("<noscript>hello! <script>alert(0)</script></noscript>").to_html)
        end

        def test_should_not_preserve_the_content_of_removed_plaintext_elements
          assert_equal("", Cleanse::DocumentFragment.new("<plaintext>hello! <script>alert(0)</script>").to_html)
        end

        def test_should_not_preserve_the_content_of_removed_script_elements
          assert_equal("", Cleanse::DocumentFragment.new("<script>hello! <script>alert(0)</script></script>").to_html)
        end

        def test_should_not_preserve_the_content_of_removed_style_elements
          assert_equal("", Cleanse::DocumentFragment.new("<style>hello! <script>alert(0)</script></style>").to_html)
        end

        def test_should_not_preserve_the_content_of_removed_svg_elements
          assert_equal("", Cleanse::DocumentFragment.new("<svg>hello! <script>alert(0)</script></svg>").to_html)
        end

        def test_should_not_preserve_the_content_of_removed_xmp_elements
          assert_equal("", Cleanse::DocumentFragment.new("<xmp>hello! <script>alert(0)</script></xmp>").to_html)
        end

        STRINGS.each do |name, data|
          define_method :"test_should_clean_#{name}_HTML" do
            assert_equal(data[:default], Cleanse::DocumentFragment.new(data[:html]).to_html)
          end
        end

        PROTOCOLS.each do |name, data|
          define_method :"test_should_not_allow_#{name}" do
            assert_equal(data[:default], Cleanse::DocumentFragment.new(data[:html]).to_html)
          end
        end
      end

      context "Restricted config" do
        def setup
          @sanitizer = Cleanse::Sanitizer.new(Cleanse::Sanitizer::Config::RESTRICTED)
        end

        STRINGS.each do |name, data|
          define_method :"test_should_clean_#{name}_HTML" do
            assert_equal(data[:restricted], Cleanse::DocumentFragment.new(data[:html], sanitizer: @sanitizer).to_html)
          end
        end

        PROTOCOLS.each do |name, data|
          define_method :"test_should_not_allow_#{name}" do
            assert_equal(data[:restricted], Cleanse::DocumentFragment.new(data[:html], sanitizer: @sanitizer).to_html)
          end
        end
      end

      context "Basic config" do
        def setup
          @sanitizer = Cleanse::Sanitizer.new(Cleanse::Sanitizer::Config::BASIC)
        end

        def test_should_not_choke_on_valueless_attributes
          assert_equal('foo <a href="">foo</a> bar',
                       Cleanse::DocumentFragment.new("foo <a href>foo</a> bar", sanitizer: @sanitizer).to_html)
        end

        def test_should_downcase_attribute_names
          assert_equal("<a>bar</a>",
                       Cleanse::DocumentFragment.new('<a HREF="javascript:alert(\'foo\')">bar</a>',
                                                     sanitizer: @sanitizer).to_html)
        end

        STRINGS.each do |name, data|
          define_method :"test_should_clean_#{name}_HTML" do
            assert_equal(data[:basic], Cleanse::DocumentFragment.new(data[:html], sanitizer: @sanitizer).to_html)
          end
        end

        PROTOCOLS.each do |name, data|
          define_method :"test_should_not_allow_#{name}" do
            assert_equal(data[:basic], Cleanse::DocumentFragment.new(data[:html], sanitizer: @sanitizer).to_html)
          end
        end
      end

      context "Relaxed config" do
        def setup
          @sanitizer = Cleanse::Sanitizer.new(Cleanse::Sanitizer::Config::RELAXED)
        end

        def test_should_encode_special_chars_in_attribute_values
          assert_equal('<a href="http://example.com" title="&lt;b&gt;éxamples&lt;/b&gt; &amp; things">foo</a>',
                       Cleanse::DocumentFragment.new('<a href="http://example.com" title="<b>&eacute;xamples</b> & things">foo</a>',
                                                     sanitizer: @sanitizer).to_html)
        end

        STRINGS.each do |name, data|
          define_method :"test_should_clean_#{name}_HTML" do
            assert_equal(data[:relaxed], Cleanse::DocumentFragment.new(data[:html], sanitizer: @sanitizer).to_html)
          end
        end

        PROTOCOLS.each do |name, data|
          define_method :"test_should_not_allow_#{name}" do
            assert_equal(data[:relaxed], Cleanse::DocumentFragment.new(data[:html], sanitizer: @sanitizer).to_html)
          end
        end
      end

      context "Custom config" do
        def test_should_allow_attributes_on_all_elements_if_allowlisted_under_all
          input = "<p>bar</p>"
          Cleanse::DocumentFragment.new(input).to_html
          assert_equal(" bar ", Cleanse::DocumentFragment.new(input).to_html)

          sanitizer = Cleanse::Sanitizer.new({
                                               elements: ["p"],
                                               attributes: { all: ["class"] }
                                             })
          assert_equal(input, Cleanse::DocumentFragment.new(input, sanitizer: sanitizer).to_html)

          sanitizer = Cleanse::Sanitizer.new({
                                               elements: ["p"],
                                               attributes: { "div" => ["class"] }
                                             })
          assert_equal("<p>bar</p>", Cleanse::DocumentFragment.new(input, sanitizer: sanitizer).to_html)

          sanitizer = Cleanse::Sanitizer.new({
                                               elements: ["p"],
                                               attributes: { "p" => ["title"], :all => ["class"] }
                                             })
          assert_equal(input, Cleanse::DocumentFragment.new(input, sanitizer: sanitizer).to_html)
        end

        def test_should_not_allow_relative_urls_when_relative_urls_arent_allowlisted
          input = '<a href="/foo/bar">Link</a>'

          sanitizer = Cleanse::Sanitizer.new({
                                               elements: ["a"],
                                               attributes: { "a" => ["href"] },
                                               protocols: { "a" => { "href" => ["http"] } }
                                             })
          assert_equal("<a>Link</a>", Cleanse::DocumentFragment.new(input, sanitizer: sanitizer).to_html)
        end

        def test_should_allow_relative_urls_containing_colons_when_the_colon_is_not_in_the_first_path_segment
          input = '<a href="/wiki/Special:Random">Random Page</a>'

          sanitizer = Cleanse::Sanitizer.new({
                                               elements: ["a"],
                                               attributes: { "a" => ["href"] },
                                               protocols: { "a" => { "href" => [:relative] } }
                                             })
          assert_equal(input, Cleanse::DocumentFragment.new(input, sanitizer: sanitizer).to_html)
        end

        def test_should_allow_relative_urls_containing_colons_when_the_colon_is_part_of_an_anchor
          input = '<a href="#fn:1">Footnote 1</a>'

          sanitizer = Cleanse::Sanitizer.new({
                                               elements: ["a"],
                                               attributes: { "a" => ["href"] },
                                               protocols: { "a" => { "href" => [:relative] } }
                                             })
          assert_equal(input, Cleanse::DocumentFragment.new(input, sanitizer: sanitizer).to_html)

          input = '<a href="somepage#fn:1">Footnote 1</a>'

          sanitizer = Cleanse::Sanitizer.new({
                                               elements: ["a"],
                                               attributes: { "a" => ["href"] },
                                               protocols: { "a" => { "href" => [:relative] } }
                                             })
          assert_equal(input, Cleanse::DocumentFragment.new(input, sanitizer: sanitizer).to_html)

          input = '<a href="fn:1">Footnote 1</a>'

          sanitizer = Cleanse::Sanitizer.new({
                                               elements: ["a"],
                                               attributes: { "a" => ["href"] },
                                               protocols: { "a" => { "href" => [:relative] } }
                                             })
          assert_equal("<a>Footnote 1</a>", Cleanse::DocumentFragment.new(input, sanitizer: sanitizer).to_html)
        end

        def test_should_remove_the_contents_of_filtered_nodes_when_remove_contents_is_true
          sanitizer = Cleanse::Sanitizer.new({ remove_contents: true })
          assert_equal("foo bar ",
                       Cleanse::DocumentFragment.new("foo bar <div>baz<span>quux</span></div>",
                                                     sanitizer: sanitizer).to_html)
        end

        def test_remove_the_contents_of_specified_nodes_when_remove_contents_is_an_Array_or_Set_of_element_names_as_strings
          sanitizer = Cleanse::Sanitizer.new({ remove_contents: %w[script span] })
          assert_equal("foo bar baz hi",
                       Cleanse::DocumentFragment.new('foo bar <div>baz<span>quux</span> <b>hi</b><script>alert("hello!");</script></div>',
                                                     sanitizer: sanitizer).to_html)

          sanitizer = Cleanse::Sanitizer.new({ remove_contents: Set.new(%w[script span]) })
          assert_equal("foo bar baz hi",
                       Cleanse::DocumentFragment.new('foo bar <div>baz<span>quux</span> <b>hi</b><script>alert("hello!");</script></div>',
                                                     sanitizer: sanitizer).to_html)
        end

        def test_should_remove_the_contents_of_specified_nodes_when_remove_contents_is_an_Array_or_Set_of_element_names_as_symbols
          sanitizer = Cleanse::Sanitizer.new({ remove_contents: %i[script span] })
          assert_equal("foo bar baz hi",
                       Cleanse::DocumentFragment.new('foo bar <div>baz<span>quux</span> <b>hi</b><script>alert("hello!");</script></div>',
                                                     sanitizer: sanitizer).to_html)

          sanitizer = Cleanse::Sanitizer.new({ remove_contents: Set.new(%i[script span]) })
          assert_equal("foo bar baz hi",
                       Cleanse::DocumentFragment.new('foo bar <div>baz<span>quux</span> <b>hi</b><script>alert("hello!");</script></div>',
                                                     sanitizer: sanitizer).to_html)
        end

        def test_should_remove_the_contents_of_allowlisted_iframes
          sanitizer = Cleanse::Sanitizer.new({ elements: ["iframe"] })
          assert_equal("<iframe> </iframe>",
                       Cleanse::DocumentFragment.new("<iframe>hi <script>hello</script></iframe>",
                                                     sanitizer: sanitizer).to_html)
        end

        def test_should_not_allow_arbitrary_HTML5_data_attributes_by_default
          sanitizer = Cleanse::Sanitizer.new({ elements: ["b"] })
          assert_equal("<b></b>", Cleanse::DocumentFragment.new('<b data-foo="bar"></b>', sanitizer: sanitizer).to_html)

          sanitizer = Cleanse::Sanitizer.new({ attributes: { "b" => ["class"] },
                                               elements: ["b"] })
          assert_equal('<b class="foo"></b>',
                       Cleanse::DocumentFragment.new('<b class="foo" data-foo="bar"></b>',
                                                     sanitizer: sanitizer).to_html)
        end

        def test_should_allow_arbitrary_HTML5_data_attributes
          sanitizer = Cleanse::Sanitizer.new(
            attributes: { "b" => %w[data-foo data-bar] },
            elements: ["b"]
          )

          assert_equal('<b data-foo="valid" data-bar="valid"></b>',
                       Cleanse::DocumentFragment.new('<b data-foo="valid" data-bar="valid"></b>',
                                                     sanitizer: sanitizer).to_html)

          assert_equal("<b></b>",
                       Cleanse::DocumentFragment.new('<b data-="invalid"></b>', sanitizer: sanitizer).to_html)

          assert_equal("<b></b>",
                       Cleanse::DocumentFragment.new('<b data-xml="invalid"></b>', sanitizer: sanitizer).to_html)

          assert_equal("<b></b>",
                       Cleanse::DocumentFragment.new('<b data-xmlfoo="invalid"></b>', sanitizer: sanitizer).to_html)

          assert_equal("<b></b>",
                       Cleanse::DocumentFragment.new('<b data-f:oo="valid"></b>', sanitizer: sanitizer).to_html)

          assert_equal("<b></b>",
                       Cleanse::DocumentFragment.new('<b data-f:oo="valid"></b>', sanitizer: sanitizer).to_html)

          assert_equal("<b></b>",
                       Cleanse::DocumentFragment.new('<b data-f/oo="partial"></b>', sanitizer: sanitizer).to_html)

          assert_equal('<b data-foo="valid"></b>',
                       Cleanse::DocumentFragment.new('<b data-éfoo="valid"></b>', sanitizer: sanitizer).to_html)
        end

        def test_should_handle_protocols_correctly_regardless_of_case
          input = '<a href="hTTpS://foo.com/">Text</a>'

          sanitizer = Cleanse::Sanitizer.new(
            elements: ["a"],
            attributes: { "a" => ["href"] },
            protocols: { "a" => { "href" => ["https"] } }
          )

          assert_equal(input, Cleanse::DocumentFragment.new(input, sanitizer: sanitizer).to_html)

          input = '<a href="mailto:someone@example.com?Subject=Hello">Text</a>'

          assert_equal("<a>Text</a>", Cleanse::DocumentFragment.new(input, sanitizer: sanitizer).to_html)
        end

        def test_should_sanitize_protocols_in_data_attributes_even_if_data_attributes_are_generically_allowed
          input = '<a data-url="mailto:someone@example.com">Text</a>'

          sanitizer = Cleanse::Sanitizer.new(
            elements: ["a"],
            attributes: { "a" => ["data-url"] },
            protocols: { "a" => { "data-url" => ["https"] } }
          )

          assert_equal("<a>Text</a>", Cleanse::DocumentFragment.new(input, sanitizer: sanitizer).to_html)

          sanitizer = Cleanse::Sanitizer.new(
            elements: ["a"],
            attributes: { "a" => ["data-url"] },
            protocols: { "a" => { "data-url" => ["mailto"] } }
          )

          assert_equal(input, Cleanse::DocumentFragment.new(input, sanitizer: sanitizer).to_html)
        end

        def test_should_prevent_meta_tags_from_being_used_to_set_a_non_UTF8_charset
          sanitizer = Cleanse::Sanitizer.new(
            elements: %w[html head meta body],
            attributes: { "meta" => ["charset"] }
          )

          assert_equal("<!DOCTYPE html><html><head><meta charset=\"utf-8\"></head><body>Howdy!</body></html>",
                       Cleanse::Document.new('<html><head><meta charset="utf-8"></head><body>Howdy!</body></html>',
                                             sanitizer: sanitizer).to_html)

          sanitizer = Cleanse::Sanitizer.new(
            elements: %w[html meta],
            attributes: { "meta" => ["charset"] }
          )

          assert_equal("<!DOCTYPE html><html><meta charset=\"utf-8\">Howdy!</html>",
                       Cleanse::Document.new('<html><meta charset="utf-8">Howdy!</html>', sanitizer: sanitizer).to_html)

          sanitizer = Cleanse::Sanitizer.new(
            elements: %w[html meta],
            attributes: { "meta" => ["charset"] }
          )

          assert_equal("<!DOCTYPE html><html><meta charset=\"utf-8\">Howdy!</html>",
                       Cleanse::Document.new('<html><meta charset="us-ascii">Howdy!</html>',
                                             sanitizer: sanitizer).to_html)

          # skip this for now
          # sanitizer = Cleanse::Sanitizer.new(
          #   elements: %w[html meta],
          #   attributes: { "meta" => %w[content http-equiv] }
          # )

          # assert_equal(
          #   "<!DOCTYPE html><html><meta http-equiv=\"content-type\" content=\" text/html;charset=utf-8\">Howdy!</html>", Cleanse::Document.new(
          #     '<html><meta http-equiv="content-type" content=" text/html; charset=us-ascii">Howdy!</html>', sanitizer: sanitizer
          #   ).to_html
          # )

          # sanitizer = Cleanse::Sanitizer.new(
          #   elements: %w[html meta],
          #   attributes: { "meta" => %w[content http-equiv] }
          # )

          # assert_equal(
          #   '<html><meta http-equiv=\"Content-Type\" content=\"text/plain;charset=utf-8\">Howdy!</html>', Cleanse::Document.new(
          #     '<html><meta http-equiv="Content-Type" content="text/plain;charset = us-ascii">Howdy!</html>', sanitizer: sanitizer
          #   ).to_html
          # )
        end

        def test_should_not_modify_meta_tags_that_already_set_a_UTF8_charset
          skip "skip this for now"
          sanitizer = Cleanse::Sanitizer.new(elements: %w[html head meta body],
                                             attributes: { "meta" => %w[content http-equiv] })

          assert_equal(
            '<html><head><meta http-equiv=\"Content-Type\" content=\"text/html;charset=utf-8\"></head><body>Howdy!</body></html>', Cleanse::Document.new(
              '<html><head><meta http-equiv="Content-Type" content="text/html;charset=utf-8"></head><body>Howdy!</body></html>', sanitizer: sanitizer
            ).to_html
          )
        end
      end
    end
  end
end
