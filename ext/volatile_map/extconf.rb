require "mkmf"

$srcs = Dir.glob("#{__dir__}/**/*.c")

have_func("rb_postponed_job_preregister", "ruby.h")

if ENV["VOLATILE_MAP_TEST_HELPERS"] == "1"
  $defs << "-DVOLATILE_MAP_TEST_HELPERS"
end

create_makefile("volatile_map")
