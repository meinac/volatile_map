# frozen_string_literal: true

require_relative "lib/volatile_map/version"

Gem::Specification.new do |spec|
  spec.name = "volatile_map"
  spec.version = VolatileMap::VERSION
  spec.authors = ["Mehmet Emin INAC"]
  spec.email = ["mehmetemininac@gmail.com"]

  spec.summary = "Implements a volatile Hash like object."
  spec.description = "This gem implement a volatile Hash like object which expires its keys based on given TTL."
  spec.homepage = "https://github.com/meinac/volatile_map"
  spec.license = "MIT"
  spec.required_ruby_version = ">= 3.2.0"
  spec.metadata["homepage_uri"] = spec.homepage
  spec.metadata["source_code_uri"] = spec.homepage

  # Specify which files should be added to the gem when it is released.
  # The `git ls-files -z` loads the files in the RubyGem that have been added into git.
  gemspec = File.basename(__FILE__)
  spec.files = IO.popen(%w[git ls-files -z], chdir: __dir__, err: IO::NULL) do |ls|
    ls.readlines("\x0", chomp: true).reject do |f|
      (f == gemspec) ||
        f.start_with?(*%w[bin/ Gemfile .gitignore .rspec spec/ .gitlab-ci.yml])
    end
  end
  spec.bindir = "exe"
  spec.executables = spec.files.grep(%r{\Aexe/}) { |f| File.basename(f) }
  spec.require_paths = ["lib"]
  spec.extensions    = ["ext/volatile_map/extconf.rb"]
end
