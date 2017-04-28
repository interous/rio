#include "error.h"
#include "types.h"
#include <stdio.h>
#include <string.h>

inline uint64_t hash(struct string_t s) {
  uint64_t n = 0xcbf29ce484222325;
  int i;
  for(i = 0; i < s.count; i++)
    n = (n ^ s.data[i]) * 0x100000001b3;
  return n;
}

struct string_t string(size_t count, char* data) {
  struct string_t res = { .count=count, .data=malloc(count) };
  strncpy(res.data, data, count);
  return res;
}

struct string_t copy_string(struct string_t s) {
  struct string_t res = { .count=s.count, .data=malloc(s.count) };
  strncpy(res.data, s.data, s.count);
  return res;
}

inline struct string_t quote_string() {
  return string(5, "quote");
}

inline struct string_t block_string() {
  return string(5, "block");
}

struct string_t int_to_string(int64_t n) {
  struct string_t res;
  res.count = snprintf(NULL, 0, "%ld", n);
  res.data = malloc(res.count + 1);
  sprintf(res.data, "%ld", n);
  return res;
}

struct string_t uint_to_string(uint64_t n) {
  struct string_t res;
  res.count = snprintf(NULL, 0, "%lu", n);
  res.data = malloc(res.count + 1);
  sprintf(res.data, "%lu", n);
  return res;
}

int string_eq(struct string_t a, struct string_t b) {
  if(a.count != b.count) return 0;
  return strncmp(a.data, b.data, a.count) == 0;
}

inline void print_string(struct string_t s) {
  int i;
  for(i = 0; i < s.count; i++) printf("%c", s.data[i]);
}

struct datum_t copy_datum(struct datum_t d) {
  struct datum_t res;
  res.tag = d.tag;
  res.file = copy_string(d.file);
  res.line = d.line;
  res.col = d.col;
  switch(d.tag) {
    case VALUE:
      res.datum.value = (struct value_t){
        .data = copy_string(d.datum.value.data),
        .type = copy_string(d.datum.value.type),
        .original = d.datum.value.original
      };
      break;
    case EINT8:
      res.datum.eint8 = (struct eint8_t){
        .data = d.datum.eint8.data,
        .type = copy_string(d.datum.eint8.type)
      };
      break;
    case BLOCK:
      res.datum.block = (struct block_t){
        .count = d.datum.block.count,
        .data = malloc(d.datum.block.count * sizeof(struct datum_t)),
        .type = copy_string(d.datum.block.type)
      };
      int i;
      for(i = 0; i < d.datum.block.count; i++)
        res.datum.block.data[i] = copy_datum(d.datum.block.data[i]);
      break;
  }
  return res;
}

void free_datum(struct datum_t d) {
  free(d.file.data);
  switch(d.tag) {
    case VALUE:
      free(d.datum.value.data.data);
      free(d.datum.value.type.data);
      break;
    case EINT8:
      free(d.datum.eint8.type.data);
      break;
    case BLOCK:
      free(d.datum.block.type.data);
      int i;
      for(i = 0; i < d.datum.block.count; i++)
        free_datum(d.datum.block.data[i]);
  }
}

struct stack_t stack;

void stack_push(struct datum_t datum) {
  stack.data[stack.used++] = datum;
  if(stack.used + 1 >= stack.capacity) {
    stack.capacity *= 2;
    stack.data = realloc(stack.data, stack.capacity * sizeof(struct stack_t));
  }
}

struct datum_t stack_pop() {
  if(stack.used == 0) empty_pop();
  struct datum_t datum = stack.data[stack.used];
  stack.used--;
  if(stack.used < stack.capacity / 3) {
    stack.capacity /= 2;
    stack.data = realloc(stack.data, stack.capacity * sizeof(struct stack_t));
  }
  return datum;
}

struct globals_t globals;

int has_global(struct string_t name) {
  size_t i = hash(name) % globals.capacity;
  while(1) {
    if(globals.data[i].tag == GLOBAL_UNSET) return 0;
    if(string_eq(name, globals.data[i].name)) return 1;
    i++;
    if(i >= globals.capacity) i = 0;
  }
}

void add_global(struct global_t global) {
  if(has_global(global.name)) duplicate_global(global.name);
  if(globals.utilization >= globals.capacity * 8 / 10) {
    struct globals_t old_globals = globals;
    globals.capacity *= 2;
    globals.utilization = 0;
    globals.data = malloc(globals.capacity * sizeof(struct global_t));
    int j;
    for(j = 0; j < globals.capacity; j++)
      globals.data[j].tag = GLOBAL_UNSET;
    for(j = 0; j < old_globals.utilization; j++)
      if(old_globals.data[j].tag != GLOBAL_UNSET)
        add_global(old_globals.data[j]);
    free(old_globals.data);
  }
  size_t i = hash(global.name) % globals.capacity;
  globals.utilization++;
  while(1) {
    if(globals.data[i].tag == GLOBAL_UNSET) {
      globals.data[i] = global;
      break;
    }
    i++;
    if(i >= globals.capacity) i = 0;
  }
}

struct global_t get_global(struct string_t name) {
  size_t i = hash(name) % globals.capacity;
  while(1) {
    if(globals.data[i].tag == GLOBAL_UNSET) {
      printf("get_global failed (this is an internal error)\n");
      exit(-1);
    }
    if(string_eq(name, globals.data[i].name)) return globals.data[i];
    i++;
    if(i >= globals.capacity) i = 0;
  }
}

struct locals_t locals;

enum local_status_t has_local(struct string_t name) {
  int i;
  for(i = locals.used; i >= 0; i--) {
    switch(locals.data[i].tag) {
      case LOCAL_SCOPE:
        return LOCAL_UNDEFINED;
      case LOCAL_BLACK:
        if(string_eq(name, locals.data[i].name))
          return LOCAL_BLACKLISTED;
        break;
      case LOCAL_DATUM:
        if(string_eq(name, locals.data[i].name))
          return LOCAL_DEFINED;
        break;
      case LOCAL_BOOKMARK:;
    }
  }
  return LOCAL_UNDEFINED;
}

void add_local(struct local_t local) {
  if(local.tag == LOCAL_DATUM) {
    if(has_global(local.name)) duplicate_global(local.name);
    enum local_status_t status = has_local(local.name);
    if(status == LOCAL_DEFINED) duplicate_local(local.name);
    if(status == LOCAL_BLACKLISTED) blacklisted_local(local.name);
  }
  else if(local.tag == LOCAL_BLACK)
    if(has_local(local.name) != LOCAL_DEFINED) invalid_blacklist(local.name);
  locals.data[locals.used++] = local;
  if(locals.used + 1 >= locals.capacity) {
    locals.capacity *= 2;
    locals.data = realloc(locals.data, locals.capacity * sizeof(struct local_t));
  }
}

// Note, get_local returns a copy of the local because it's expected to be
// modified and, e.g., clearing the local bindings shouldn't impact the stack
// "at first order"
struct datum_t get_local(struct string_t name) {
  int i;
  for(i = locals.used; i >= 0; i--) {
    switch(locals.data[i].tag) {
      case LOCAL_SCOPE:
        printf("get_local: scope boundary (this is an internal error)\n");
        exit(-1);
      case LOCAL_BLACK:
        if(string_eq(name, locals.data[i].name)) {
          printf("get_local: name blacklisted (this is an internal error)\n");
          exit(-1);
        }
      case LOCAL_DATUM:
        if(string_eq(name, locals.data[i].name))
          return copy_datum(locals.data[i].data.datum);
      case LOCAL_BOOKMARK:;
    }
  }
  printf("get_local: not found (this is an internal error)\n");
  exit(-1);
}

void clear_to_bookmark() {
  int i, j;
  for(i = locals.used; i >= 0; i--) {
    free(locals.data[i].name.data);
    if(locals.data[i].tag == LOCAL_BOOKMARK) {
      locals.used = i - 1;
      break;
    }
    else if(locals.data[i].tag == LOCAL_DATUM) {
      if(locals.data[i].data.datum.tag == VALUE)
        for(j = 0; j < stack.used; j++)
          if(stack.data[i].tag == VALUE &&
              string_eq(stack.data[i].datum.value.data, locals.data[i].data.datum.datum.value.data)) {
            aliased_deletion(locals.data[i].name);
          }
      free_datum(locals.data[i].data.datum);
    }
  }
}

void clear_to_scope() {
  int i, j;
  for(i = locals.used; i >= 0; i--) {
    free(locals.data[i].name.data);
    if(locals.data[i].tag == LOCAL_SCOPE) {
      locals.used = i - 1;
      break;
    }
    else if(locals.data[i].tag == LOCAL_DATUM) {
      if(locals.data[i].data.datum.tag == VALUE)
        for(j = 0; j < stack.used; j++)
          if(stack.data[i].tag == VALUE &&
              string_eq(stack.data[i].datum.value.data, locals.data[i].data.datum.datum.value.data)) {
            aliased_deletion(locals.data[i].name);
          }
      free_datum(locals.data[i].data.datum);
    }
  }
}

struct stars_t stars;

int has_star(struct string_t name) {
  size_t i = hash(name) % stars.capacity;
  while(1) {
    if(stars.data[i].status == STAR_EMPTY) return 0;
    if(string_eq(name, stars.data[i].name)) return 1;
    i++;
    if(i >= stars.capacity) i = 0;
  }
}

void add_star(struct star_t star) {
  if(has_star(star.name)) duplicate_local(star.name);
  if(stars.utilization >= stars.capacity * 8 / 10) {
    struct stars_t old_stars = stars;
    stars.capacity *= 2;
    stars.utilization = 0;
    stars.data = malloc(stars.capacity * sizeof(struct star_t));
    int j;
    for(j = 0; j < stars.capacity; j++)
      stars.data[j].status = STAR_EMPTY;
    for(j = 0; j < old_stars.utilization; j++)
      if(old_stars.data[j].status != STAR_EMPTY)
        add_star(old_stars.data[j]);
    free(old_stars.data);
  }
  size_t i = hash(star.name) % stars.capacity;
  while(1) {
    if(stars.data[i].status == STAR_EMPTY || stars.data[i].status == STAR_DELETED) {
      if(stars.data[i].status == STAR_EMPTY) stars.utilization++;
      stars.data[i] = star;
      break;
    }
    i++;
    if(i >= stars.capacity) i = 0;
  }
}

struct datum_t get_star(struct string_t name) {
  size_t i = hash(name) % stars.capacity;
  while(1) {
    if(stars.data[i].status != STAR_INHABITED) {
      printf("get_star failed (this is an internal error)\n");
      exit(-1);
    }
    if(string_eq(name, stars.data[i].name)) {
      free(stars.data[i].name.data);
      return stars.data[i].data;
    }
    i++;
    if(i >= stars.capacity) i = 0;
  }
}
