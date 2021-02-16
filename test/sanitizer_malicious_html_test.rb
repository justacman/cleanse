# frozen_string_literal: true

require "test_helper"

module Cleanse
  class SanitizerMaliciousHtmlTest < Minitest::Test
    def setup
      @sanitizer = Cleanse::Sanitizer.new(Sanitizer::Config::RELAXED)
    end

    def test_should_not_allow_script_injection_via_conditional_comments
      assert_equal("",
                   Cleanse::DocumentFragment.new(%[<!--[if gte IE 4]>\n<script>alert('XSS');</script>\n<![endif]-->],
                                                 sanitizer: @sanitizer).to_html)
    end

    def test_should_escape_ERB_style_tags
      assert_equal("&lt;% naughty_ruby_code %&gt;",
                   Cleanse::DocumentFragment.new("<% naughty_ruby_code %>", sanitizer: @sanitizer).to_html)

      assert_equal("&lt;%= naughty_ruby_code %&gt;",
                   Cleanse::DocumentFragment.new("<%= naughty_ruby_code %>", sanitizer: @sanitizer).to_html)
    end

    def test_should_remove_PHP_style_tags
      assert_equal("", Cleanse::DocumentFragment.new("<? naughtyPHPCode(); ?>", sanitizer: @sanitizer).to_html)

      assert_equal("", Cleanse::DocumentFragment.new("<?= naughtyPHPCode(); ?>", sanitizer: @sanitizer).to_html)
    end

    def test_should_not_be_possible_to_inject_JS_via_a_malformed_event_attribute
      assert_equal("<!DOCTYPE html><html><head></head><body></body></html>",
                   Cleanse::Document.new('<html><head></head><body onload!#$%&()*~+-_.,:;?@[/|\\]^`=alert("XSS")></body></html>',
                                         sanitizer: @sanitizer).to_html)
    end

    def test_should_not_be_possible_to_inject_an_iframe_using_an_improperly_closed_tag
      assert_equal("",
                   Cleanse::DocumentFragment.new(%(<iframe src=http://ha.ckers.org/scriptlet.html <),
                                                 sanitizer: @sanitizer).to_html)
    end

    def test_should_not_be_possible_to_inject_JS_via_an_unquoted_img_src_attribute
      assert_equal("<img>",
                   Cleanse::DocumentFragment.new('<img src=javascript:alert("XSS")>', sanitizer: @sanitizer).to_html)
    end

    def test_should_not_be_possible_to_inject_JS_using_grave_accents_as_img_src_delimiters
      assert_equal("<img>",
                   Cleanse::DocumentFragment.new('<img src=`javascript:alert("XSS")`>', sanitizer: @sanitizer).to_html)
    end

    def test_should_not_be_possible_to_inject_script_via_a_malformed_img_tag
      assert_equal('<img>"&gt;',
                   Cleanse::DocumentFragment.new('<img """><script>alert("XSS")</script>">',
                                                 sanitizer: @sanitizer).to_html)
    end

    def test_should_not_be_possible_to_inject_protocol_based_JS
      assert_equal("<img>",
                   Cleanse::DocumentFragment.new(
                     "<img src=&#106;&#97;&#118;&#97;&#115;&#99;&#114;&#105;&#112;&#116;&#58;&#97;&#108;&#101;&#114;&#116;&#40;&#39;&#88;&#83;&#83;&#39;&#41;>", sanitizer: @sanitizer
                   ).to_html)

      assert_equal("<img>",
                   Cleanse::DocumentFragment.new(
                     "<img src=&#0000106&#0000097&#0000118&#0000097&#0000115&#0000099&#0000114&#0000105&#0000112&#0000116&#0000058&#0000097&#0000108&#0000101&#0000114&#0000116&#0000040&#0000039&#0000088&#0000083&#0000083&#0000039&#0000041>", sanitizer: @sanitizer
                   ).to_html)

      assert_equal("<img>",
                   Cleanse::DocumentFragment.new(
                     "<img src=&#x6A&#x61&#x76&#x61&#x73&#x63&#x72&#x69&#x70&#x74&#x3A&#x61&#x6C&#x65&#x72&#x74&#x28&#x27&#x58&#x53&#x53&#x27&#x29>", sanitizer: @sanitizer
                   ).to_html)

      # Encoded tab character.
      assert_equal("<img>",
                   Cleanse::DocumentFragment.new(%[<img src="jav&#x09;ascript:alert('XSS');">],
                                                 sanitizer: @sanitizer).to_html)

      # Encoded newline.
      assert_equal("<img>",
                   Cleanse::DocumentFragment.new(%[<img src="jav&#x0A;ascript:alert('XSS');">],
                                                 sanitizer: @sanitizer).to_html)

      # Encoded carriage return.
      assert_equal("<img>",
                   Cleanse::DocumentFragment.new(%[<img src="jav&#x0D;ascript:alert('XSS');">],
                                                 sanitizer: @sanitizer).to_html)

      # Null byte.
      assert_equal("<img>",
                   Cleanse::DocumentFragment.new(%[<img src=java\0script:alert("XSS")>], sanitizer: @sanitizer).to_html)

      # Spaces plus meta char.
      assert_equal("<img>",
                   Cleanse::DocumentFragment.new(%[<img src=" &#14;  javascript:alert('XSS');">],
                                                 sanitizer: @sanitizer).to_html)

      # Mixed spaces and tabs.
      assert_equal("<img>",
                   Cleanse::DocumentFragment.new(%[<img src="j\na v\tascript://alert('XSS');">],
                                                 sanitizer: @sanitizer).to_html)
    end

    def test_should_not_be_possible_to_inject_protocol_based_JS_via_whitespace
      assert_equal("<img>",
                   Cleanse::DocumentFragment.new(%[<img src="jav\tascript:alert('XSS');">],
                                                 sanitizer: @sanitizer).to_html)
    end

    def test_should_not_be_possible_to_inject_JS_using_a_half_open_img_tag
      assert_equal("",
                   Cleanse::DocumentFragment.new(%[<img src="javascript:alert('XSS')"], sanitizer: @sanitizer).to_html)
    end

    def test_should_not_be_possible_to_inject_script_using_a_malformed_non_alphanumeric_tag_name
      assert_equal("",
                   Cleanse::DocumentFragment.new(%[<script/xss src="http://ha.ckers.org/xss.js">alert(1)</script>],
                                                 sanitizer: @sanitizer).to_html)
    end

    def test_should_not_be_possible_to_inject_script_via_extraneous_open_brackets
      assert_equal("&lt;",
                   Cleanse::DocumentFragment.new(%[<<script>alert("XSS");//<</script>], sanitizer: @sanitizer).to_html)
    end

    # https://github.com/rgrove/sanitize/security/advisories/GHSA-p4x4-rw2p-8j8m
    def test_prevents_a_sanitization_bypass_via_carefully_crafted_foreign_content
      %w[iframe noembed noframes noscript plaintext script style xmp].each do |tag_name|
        assert_equal("",
                     Cleanse::DocumentFragment.new(%[<math><#{tag_name}>/*&lt;/#{tag_name}&gt;&lt;img src onerror=alert(1)>*/],
                                                   sanitizer: @sanitizer).to_html)

        assert_equal("",
                     Cleanse::DocumentFragment.new(%[<svg><#{tag_name}>/*&lt;/#{tag_name}&gt;&lt;img src onerror=alert(1)>*/],
                                                   sanitizer: @sanitizer).to_html)
      end
    end
  end
end
