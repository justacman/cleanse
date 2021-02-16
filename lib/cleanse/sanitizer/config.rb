# frozen_string_literal: true

require "set"

module Cleanse
  class Sanitizer
    module Config
      # Deeply freezes and returns the given configuration Hash.
      def self.freeze_config(config)
        case config
        when Hash
          config.each_value { |c| freeze_config(c) }
        when Array, Set
          config.each { |c| freeze_config(c) }
        end

        config.freeze
      end

      # Returns a new Hash containing the result of deeply merging *other_config*
      # into *config*. Does not modify *config* or *other_config*.
      #
      # This is the safest way to use a built-in Sanitize config as the basis for
      # your own custom config.
      def self.merge(config, other_config = {})
        raise ArgumentError, "config must be a Hash" unless config.is_a?(Hash)
        raise ArgumentError, "other_config must be a Hash" unless other_config.is_a?(Hash)

        merged = {}
        keys   = Set.new(config.keys + other_config.keys)

        keys.each do |key|
          oldval = config[key]

          if other_config.key?(key)
            newval = other_config[key]

            merged[key] = if oldval.is_a?(Hash) && newval.is_a?(Hash)
                            oldval.empty? ? newval.dup : merge(oldval, newval)
                          elsif newval.is_a?(Array) && key != :transformers
                            Set.new(newval)
                          else
                            can_dupe?(newval) ? newval.dup : newval
                          end
          else
            merged[key] = can_dupe?(oldval) ? oldval.dup : oldval
          end
        end

        merged
      end

      # Returns `true` if `dup` may be safely called on _value_, `false`
      # otherwise.
      def self.can_dupe?(value)
        !(value == true || value == false || value.nil? || value.is_a?(Method) || value.is_a?(Numeric) || value.is_a?(Symbol))
      end
      private_class_method :can_dupe?
    end
  end
end

require "cleanse/sanitizer/config/basic"
require "cleanse/sanitizer/config/default"
require "cleanse/sanitizer/config/relaxed"
require "cleanse/sanitizer/config/restricted"
