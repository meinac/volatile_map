#include "test_helpers.h"

static double test_time;

int test_clock_gettime(clockid_t _clk_id, struct timespec *tp) {
  tp->tv_sec = (int)test_time;
  tp->tv_nsec = 0;

  return 0;
}

VALUE vm_set_time(VALUE _klass, VALUE time) {
  test_time = NUM2DBL(time);

  return time;
}

void init_test_helpers(VALUE klass) {
  rb_define_singleton_method(klass, "set_test_clock!", vm_set_time, 1);
}
