# frozen_string_literal: true

require "bundler/gem_tasks"
require "rspec/core/rake_task"
require "rake/extensiontask"

ENV["VOLATILE_MAP_TEST_HELPERS"] = "1"

RSpec::Core::RakeTask.new(:spec)

Rake::ExtensionTask.new "volatile_map" do |ext|
  ext.lib_dir = "lib"
  ext.ext_dir = 'ext/volatile_map'
end

task default: [:compile, :spec]
