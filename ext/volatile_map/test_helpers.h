#include <ruby.h>
#include <time.h>

int test_clock_gettime(clockid_t, struct timespec *);
void init_test_helpers(VALUE klass);

#define clock_gettime test_clock_gettime
