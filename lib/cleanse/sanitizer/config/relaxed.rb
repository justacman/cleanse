# frozen_string_literal: true

module Cleanse
  class Sanitizer
    module Config
      RELAXED = freeze_config(
        elements: BASIC[:elements] + %w[
          address article aside bdi bdo body caption col colgroup data del div
          figcaption figure footer h1 h2 h3 h4 h5 h6 head header hgroup hr html
          img ins main nav rp rt ruby section span style summary sup table tbody
          td tfoot th thead title tr wbr
        ],

        allow_doctype: true,

        attributes: merge(BASIC[:attributes],
                          :all => %w[class dir hidden id lang style tabindex title translate],
                          "a" => %w[href hreflang name rel],
                          "col" => %w[span width],
                          "colgroup" => %w[span width],
                          "data" => %w[value],
                          "del" => %w[cite datetime],
                          "img" => %w[align alt border height src srcset width],
                          "ins" => %w[cite datetime],
                          "li" => %w[value],
                          "ol" => %w[reversed start type],
                          "style" => %w[media scoped type],
                          "table" => %w[align bgcolor border cellpadding cellspacing frame rules sortable summary
                                        width],
                          "td" => %w[abbr align axis colspan headers rowspan valign width],
                          "th" => %w[abbr align axis colspan headers rowspan scope sorted valign width],
                          "ul" => %w[type]),

        protocols: merge(BASIC[:protocols],
                         "del" => { "cite" => ["http", "https", :relative] },
                         "img" => { "src"  => ["http", "https", :relative] },
                         "ins" => { "cite" => ["http", "https", :relative] })
      )
    end
  end
end
