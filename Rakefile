# frozen_string_literal: true

require "rake/clean"
require "rake/extensiontask"
require "rake/testtask"

task default: :test

ext = Rake::ExtensionTask.new "cleanse" do |e|
  e.lib_dir = File.join('lib', 'cleanse')
  e.source_pattern = "{,nokogumbo/gumbo-parser/src/}*.[hc]"
end

task "test" => [:compile]
