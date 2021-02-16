# frozen_string_literal: true

require 'cleanse/cleanse'

require "cleanse/version"
require "cleanse/sanitizer"

module Cleanse
  class Document
    attr_reader :sanitizer

    def to_html
      Serializer.new(self).to_html
    end
  end

  class DocumentFragment
    attr_reader :sanitizer

    def to_html
      Serializer.new(self).to_html
    end
  end
end
