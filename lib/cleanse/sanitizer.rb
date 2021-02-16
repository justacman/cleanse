# frozen_string_literal: true

require_relative "sanitizer/config"

module Cleanse
  class Sanitizer
    # initialize is in C, this just helps manage config setup in Ruby
    private def setup(config)
      @config = config

      allow_element(config[:elements] || [])

      (config[:attributes] || {}).each do |element, attrs|
        allow_attribute element, attrs
      end

      (config[:protocols] || {}).each do |element, protocols|
        protocols.each do |attribute, pr|
          allow_protocol element, attribute, pr
        end
      end

      remove_contents(config[:remove_contents]) if config.include?(:remove_contents)

      wrap_with_whitespace(config[:whitespace_elements]) if config.include?(:whitespace_elements)

      set_allow_comments(config.fetch(:allow_comments, false))
      set_allow_doctype(config.fetch(:allow_doctype, true))
    end

    def elements
      @config[:elements]
    end

    def allow_element(elements)
      elements.flatten.each { |e| set_flag e, ALLOW, true }
    end

    def disallow_element(elements)
      elements.flatten.each { |e| set_flag e, ALLOW, false }
    end

    def allow_attribute(element, attrs)
      attrs.flatten.each { |attr| set_allowed_attribute element, attr, true }
    end

    def require_any_attributes(element, attrs)
      if attr.empty?
        set_required_attribute element, "*", true
      else
        attrs.flatten.each { |attr| set_required_attribute element, attr, true }
      end
    end

    def disallow_attribute(element, attrs)
      attrs.flatten.each { |attr| set_allowed_attribute element, attr, false }
    end

    def allow_class(element, *klass)
      klass.flatten.each { |k| set_allowed_class element, k, true }
    end

    def allow_protocol(element, attr, protos)
      protos = [protos] unless protos.is_a?(Array)
      set_allowed_protocols element, attr, protos
    end

    def remove_contents(elements)
      if elements.is_a?(TrueClass) || elements.is_a?(FalseClass)
        set_all_flags REMOVE_CONTENTS, elements
      else
        elements.flatten.each { |e| set_flag e, REMOVE_CONTENTS, true }
      end
    end

    def wrap_with_whitespace(elements)
      elements.flatten.each { |e| set_flag e, WRAP_WHITESPACE, true }
    end
  end
end
