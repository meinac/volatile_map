#include <time.h>
#include "volatile_map.h"

#ifdef VOLATILE_MAP_TEST_HELPERS
#include "test_helpers.h"
#endif

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

static VolatileMap **vm_dirty_array = NULL;
static size_t vm_dirty_len = 0;
static size_t vm_dirty_capacity = 0;
static size_t vm_live_count = 0;

#ifdef HAVE_RB_POSTPONED_JOB_PREREGISTER
static rb_postponed_job_handle_t vm_drain_job_handle;
#endif

struct vm_drain_ctx {
  double now;
  double ttl;
};

static double now_seconds(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);

  return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static bool is_stale(double ttl, VALUE entry, double now) {
  double ts = NUM2DBL(RARRAY_AREF(entry, 1));

  return (now - ts) > ttl;
}

static int drain_entry(VALUE _key, VALUE entry, VALUE arg) {
  struct vm_drain_ctx *ctx = (struct vm_drain_ctx *)arg;

  if(!RB_TYPE_P(entry, T_ARRAY) || RARRAY_LEN(entry) < 2) return ST_CONTINUE;

  if(is_stale(ctx->ttl, entry, ctx->now)) return ST_DELETE;

  return ST_CONTINUE;
}

static void vm_drain_dirty(void *_ptr) {
  double now = now_seconds();

  for(size_t i = 0; i < vm_dirty_len; i++) {
    VolatileMap *vm = vm_dirty_array[i];

    if(RHASH_SIZE(vm->storage) == 0) continue;

    struct vm_drain_ctx ctx = { now, vm->ttl };

    rb_hash_foreach(vm->storage, drain_entry, (VALUE)&ctx);
  }

  vm_dirty_len = 0;
}

static inline void schedule_draining(void) {
  #ifdef HAVE_RB_POSTPONED_JOB_PREREGISTER
    rb_postponed_job_trigger(vm_drain_job_handle);
  #else
    rb_postponed_job_register_one(0, vm_drain_dirty, NULL);
  #endif
}

static void rb_mark_volatile_map(void *ptr) {
  VolatileMap *vm = (VolatileMap *)ptr;

  rb_gc_mark_movable(vm->storage);

  if(RHASH_SIZE(vm->storage) > 0 && vm_dirty_len < vm_dirty_capacity) {
    vm_dirty_array[vm_dirty_len++] = vm;

    schedule_draining();
  }
}

static void rb_free_volatile_map(void *ptr) {
  VolatileMap *vm = (VolatileMap *)ptr;

  xfree(vm);

  if(vm_live_count > 0) vm_live_count--;
}

static void rb_compact_volatile_map(void *ptr) {
  VolatileMap *vm = (VolatileMap *)ptr;

  vm->storage = rb_gc_location(vm->storage);
}

static size_t rb_size_volatile_map(const void *ptr) {
  VolatileMap *vm = (VolatileMap *)ptr;

  return sizeof(VolatileMap) + rb_funcall(vm->storage, rb_intern("size"), 0);
}

const rb_data_type_t volatile_map_type = {
  "VolatileMap",
  {
    rb_mark_volatile_map,
    rb_free_volatile_map,
    rb_size_volatile_map,
    rb_compact_volatile_map
  },
  0,
  0,
  RUBY_TYPED_FREE_IMMEDIATELY
};

static void ensure_dirty_capacity(void) {
  if(vm_dirty_capacity >= vm_live_count) return;

  size_t new_cap = MAX((size_t)8, vm_live_count * 2);

  vm_dirty_array = (VolatileMap **)xrealloc(vm_dirty_array, new_cap * sizeof(VolatileMap *));
  vm_dirty_capacity = new_cap;
}

VALUE alloc_volatile_map(VALUE klass) {
  VolatileMap *vm;
  VALUE obj = TypedData_Make_Struct(klass, VolatileMap, &volatile_map_type, vm);

  vm->storage = rb_hash_new();
  vm->ttl = 0.0;
  vm_live_count++;

  ensure_dirty_capacity();

  return obj;
}

static VolatileMap *vm_get(VALUE self) {
  VolatileMap *vm;
  TypedData_Get_Struct(self, VolatileMap, &volatile_map_type, vm);

  return vm;
}

static VALUE vm_initialize(VALUE self, VALUE ttl) {
  VolatileMap *vm = vm_get(self);
  double ttl_value = NUM2DBL(ttl);

  if(ttl_value <= 0.0) rb_raise(rb_eArgError, "TTL must be positive");

  vm->ttl = ttl_value;

  return self;
}

static VALUE vm_ttl(VALUE self) {
  return DBL2NUM(vm_get(self)->ttl);
}

static VALUE vm_aset(VALUE self, VALUE key, VALUE value) {
  VolatileMap *vm = vm_get(self);
  VALUE entry = rb_ary_new_from_args(2, value, DBL2NUM(now_seconds()));

  rb_hash_aset(vm->storage, key, entry);

  return value;
}

static VALUE vm_aref(VALUE self, VALUE key) {
  VolatileMap *vm = vm_get(self);
  VALUE entry = rb_hash_lookup2(vm->storage, key, Qundef);

  if(entry == Qundef) return Qnil;

  double now = now_seconds();

  if(is_stale(vm->ttl, entry, now)) {
    rb_hash_delete(vm->storage, key);

    return Qnil;
  }

  rb_ary_store(entry, 1, DBL2NUM(now));

  return RARRAY_AREF(entry, 0);
}

static VALUE vm_delete(VALUE self, VALUE key) {
  VolatileMap *vm = vm_get(self);
  VALUE entry = rb_hash_delete(vm->storage, key);

  if(NIL_P(entry) || !RB_TYPE_P(entry, T_ARRAY) || RARRAY_LEN(entry) < 1) return Qnil;

  return RARRAY_AREF(entry, 0);
}

static VALUE vm_size(VALUE self) {
  VolatileMap *vm = vm_get(self);

  if(NIL_P(vm->storage)) return INT2FIX(0);

  return ULONG2NUM(RHASH_SIZE(vm->storage));
}

static VALUE vm_key(VALUE self, VALUE key) {
  VolatileMap *vm = vm_get(self);
  VALUE entry = rb_hash_lookup2(vm->storage, key, Qundef);

  if(entry == Qundef) return Qfalse;

  double now = now_seconds();

  if(is_stale(vm->ttl, entry, now)) {
    rb_hash_delete(vm->storage, key);

    return Qfalse;
  }

  rb_ary_store(entry, 1, DBL2NUM(now));

  return Qtrue;
}

void Init_volatile_map(void) {
  rb_require("volatile_map/version");

  VALUE volatile_map_class = rb_define_class("VolatileMap", rb_cObject);

  rb_define_alloc_func(volatile_map_class, alloc_volatile_map);

  rb_define_method(volatile_map_class, "initialize", vm_initialize, 1);
  rb_define_method(volatile_map_class, "ttl", vm_ttl, 0);
  rb_define_method(volatile_map_class, "[]=", vm_aset, 2);
  rb_define_method(volatile_map_class, "[]", vm_aref, 1);
  rb_define_method(volatile_map_class, "delete", vm_delete, 1);
  rb_define_method(volatile_map_class, "size", vm_size, 0);
  rb_define_method(volatile_map_class, "length", vm_size, 0);
  rb_define_method(volatile_map_class, "key?", vm_key, 1);
  rb_define_method(volatile_map_class, "has_key?", vm_key, 1);
  rb_define_method(volatile_map_class, "include?", vm_key, 1);
  rb_define_method(volatile_map_class, "member?", vm_key, 1);

#ifdef VOLATILE_MAP_TEST_HELPERS
  init_test_helpers(volatile_map_class);
#endif

#ifdef HAVE_RB_POSTPONED_JOB_PREREGISTER
  vm_drain_job_handle = rb_postponed_job_preregister(0, vm_drain_dirty, NULL);
#endif
}
