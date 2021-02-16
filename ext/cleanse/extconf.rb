# frozen_string_literal: true

# rubocop:disable Style/GlobalVars

require "mkmf"

CONFIG["optflags"] = "-O2"

$CFLAGS << " -std=c99 -Werror-implicit-function-declaration -Wno-declaration-after-statement -fvisibility=hidden"
$LDFLAGS.gsub!("-Wl,--no-undefined", "")
$warnflags = CONFIG["warnflags"] = "-Wall"

EXT_DIR = File.dirname(__FILE__)
GUMBO_DIR = File.expand_path(File.join(EXT_DIR, "nokogumbo", "gumbo-parser"))
GUMBO_SRC_DIR = File.expand_path(File.join(GUMBO_DIR, "src"))

find_header("gumbo.h", GUMBO_SRC_DIR)
find_header("parser.h", GUMBO_SRC_DIR)
find_header("string_buffer.h", GUMBO_SRC_DIR)

# Symlink gumbo-parser source files.
Dir.chdir(EXT_DIR) do
  $srcs = Dir["*.c", "nokogumbo/gumbo-parser/src/*.c"]
  $hdrs = Dir["*.h", "nokogumbo/gumbo-parser/src/*.h"]
end
$INCFLAGS << " -I$(srcdir)/nokogumbo/gumbo-parser/src"
$VPATH << "$(srcdir)/nokogumbo/gumbo-parser/src"

create_makefile("cleanse/cleanse") do |conf|
  conf.map! do |chunk|
    chunk.gsub(/^HDRS = .*$/, "HDRS = #{$hdrs.map { |h| File.join("$(srcdir)", h) }.join(" ")}")
  end
end

# rubocop:enable Style/GlobalVars
