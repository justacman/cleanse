# frozen_string_literal: true

module Cleanse
  class Sanitizer
    module Config
      RESTRICTED = freeze_config(
        elements: %w[b em i strong u],

        whitespace_elements: DEFAULT[:whitespace_elements]
      )
    end
  end
end
