#!ruby

MRuby::Gem::Specification.new("mruby-delta") do |s|
  s.summary = "calculate the difference between adjacent byte values"
  s.version = File.read(File.join(File.dirname(__FILE__), "README.md")).scan(/^ *[-*] version: *(\d+(?:.\w+)+)/i).flatten[-1]
  s.license = "BSD-2-Clause"
  s.author  = "dearblue"
  s.homepage = "https://github.com/dearblue/mruby-delta"

  add_dependency "mruby-aux", github: "dearblue/mruby-aux"

  if cc.command =~ /\b(?:g?cc|clang)\d*\b/
    cc.flags << %w(-Wno-declaration-after-statement -Wno-error-declaration-after-statement)
  end
end
