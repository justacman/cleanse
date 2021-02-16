# frozen_string_literal: true
lib = File.expand_path('../lib', __FILE__)
$LOAD_PATH.unshift(lib) unless $LOAD_PATH.include?(lib)
require 'cleanse/version'

Gem::Specification.new do |spec|
  spec.name          = "cleanse"
  spec.version       = Cleanse::VERSION
  spec.authors       = [""]
  spec.email         = [""]

  spec.summary       = "Write a short summary, because RubyGems requires one."
  spec.description   = "Write a longer description or delete this line."

  spec.license       = "MIT"
  spec.required_ruby_version = Gem::Requirement.new(">= 2.5.0")

  spec.files = %w[
    Rakefile
    cleanse.gemspec
    lib/cleanse.rb
    lib/cleanse/sanitizer.rb
  ]
  spec.files += Dir.glob("ext/cleanse/*.{c,h}")

  spec.test_files = []
  spec.extensions = ["ext/cleanse/extconf.rb"]
  spec.require_paths = ["lib"]
end
