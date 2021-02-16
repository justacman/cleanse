# frozen_string_literal: true

module Cleanse
  class Sanitizer
    module Config
      BASIC = freeze_config(
        elements: %w[
          a abbr blockquote b br cite code dd dfn dl dt em i kbd li mark ol p pre q s
          samp small strike strong sub sup time u ul var
        ],

        attributes: {
          "a" => %w[href],
          "abbr" => %w[title],
          "blockquote" => %w[cite],
          "dfn" => %w[title],
          "q" => %w[cite],
          "time" => %w[datetime pubdate]
        },

        protocols: {
          "a" => { "href" => ["ftp", "http", "https", "mailto", :relative] },
          "blockquote" => { "cite" => ["http", "https", :relative] },
          "q" => { "cite" => ["http", "https", :relative] }
        }
      )
    end
  end
end
