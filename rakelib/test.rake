# frozen_string_literal: true

require "rake/testtask"

#
#  much of this was ripped out of hoe-debugging
#
class ValgrindTestTask < Rake::TestTask
  DEFAULT_DIRECTORY_NAME = "suppressions"
  ERROR_EXITCODE = 42 # the answer to life, the universe, and segfaulting.
  VALGRIND_OPTIONS = ["--num-callers=50",
                      "--error-limit=no",
                      "--partial-loads-ok=yes",
                      "--undef-value-errors=no",
                      "--error-exitcode=#{ERROR_EXITCODE}",
                      "--gen-suppressions=all"].freeze

  def ruby(*args, **options, &block)
    valgrind_options = check_for_suppression_file(VALGRIND_OPTIONS)
    command = "valgrind #{valgrind_options.join(" ")} #{RUBY} #{args.join(" ")}"
    sh(command, **options, &block)
  end

  def formatted_ruby_version
    engine = if defined?(RUBY_DESCRIPTION) && RUBY_DESCRIPTION =~ /Ruby Enterprise Edition/
               "ree"
             else
               defined?(RUBY_ENGINE) ? RUBY_ENGINE : "ruby"
             end
    %(#{engine}-#{RUBY_VERSION}.#{RUBY_PATCHLEVEL})
  end

  def check_for_suppression_file(options)
    options = options.dup
    suppression_files = matching_suppression_files
    suppression_files.each do |suppression_file|
      puts "NOTICE: using valgrind suppressions in #{suppression_file.inspect}"
      options << "--suppressions=#{suppression_file}"
    end
    options
  end

  def matching_suppression_files
    matching_files = []
    version_matches.each do |version_string|
      matching_files += Dir[File.join(DEFAULT_DIRECTORY_NAME, "cleanse_#{version_string}.supp")]
      matching_files += Dir[File.join(DEFAULT_DIRECTORY_NAME, "cleanse_#{version_string}_*.supp")]
    end
    matching_files
  end

  def version_matches
    matches = [formatted_ruby_version] # e.g. "ruby-2.5.1.57"
    matches << formatted_ruby_version.split(".")[0, 3].join(".") # e.g. "ruby-2.5.1"
    matches << formatted_ruby_version.split(".")[0, 2].join(".") # e.g. "ruby-2.5"
    matches << formatted_ruby_version.split(".")[0, 1].join(".") # e.g. "ruby-2"
    matches
  end
end

class GdbTestTask < ValgrindTestTask
  def ruby(*args, **options, &block)
    command = "gdb --args #{RUBY} #{args.join(" ")}"
    sh(command, **options, &block)
  end
end

class LldbTestTask < ValgrindTestTask
  def ruby(*args, **options, &block)
    command = "lldb -- #{RUBY} #{args.join(" ")}"
    sh(command, **options, &block)
  end
end

def cleanse_test_task_configuration(t)
  t.libs << "lib"
  t.libs << "test"
  t.pattern = "test/**/*_test.rb"
  t.verbose = true
  t.warning = false
end

Rake::TestTask.new do |t|
  cleanse_test_task_configuration(t)
end

namespace "test" do
  ValgrindTestTask.new("valgrind") do |t|
    cleanse_test_task_configuration(t)
  end

  GdbTestTask.new("gdb") do |t|
    cleanse_test_task_configuration(t)
  end

  LldbTestTask.new("lldb") do |t|
    cleanse_test_task_configuration(t)
  end
end
