#ifndef VOLATILE_MAP_H
#define VOLATILE_MAP_H

#include <ruby.h>
#include <ruby/debug.h>

typedef struct volatile_map_struct {
  VALUE storage;
  double ttl;
} VolatileMap;

VALUE alloc_volatile_map(VALUE klass);
void Init_volatile_map(void);

#endif
