#pragma once
#include <stdint.h>
#include <stdlib.h>

struct string_t {
  size_t count;
  char* data;
};

struct string_t string(size_t count, char* data);
struct string_t copy_string(struct string_t s);
struct string_t quote_string();
struct string_t block_string();
struct string_t int_to_string(int64_t n);
struct string_t uint_to_string(uint64_t n);

int string_eq(struct string_t a, struct string_t b);
void print_string(struct string_t s);

struct value_t {
  struct string_t data;
  struct string_t type;
  int original;
};

struct eint8_t {
  int64_t data;
  struct string_t type;
};

struct block_t {
  size_t count;
  struct datum_t* data;
  struct string_t type;
};

enum datum_tag_t {
  VALUE, EINT8, BLOCK
};

union datum_union_t {
  struct value_t value;
  struct eint8_t eint8;
  struct block_t block;
};

struct datum_t {
  enum datum_tag_t tag;
  union datum_union_t datum;
  struct string_t file;
  uint64_t line, col;
};

struct datum_t copy_datum(struct datum_t d);
void free_datum(struct datum_t d);

struct stack_t {
  size_t used, capacity;
  struct datum_t* data;
};

extern struct stack_t stack;
void stack_push(struct datum_t datum);
struct datum_t stack_pop();

enum global_tag_t {
  GLOBAL_UNSET, GLOBAL_BLOCK, GLOBAL_FUNCTION, GLOBAL_RESERVED
};

union global_union_t {
  struct block_t block;
  void* none;
};

struct global_t {
  struct string_t name;
  enum global_tag_t tag;
  union global_union_t data;
};

struct globals_t {
  size_t capacity, utilization;
  struct global_t* data;
};

extern struct globals_t globals;
int has_global(struct string_t name);
void add_global(struct global_t global);
struct global_t get_global(struct string_t name);

enum local_tag_t {
  LOCAL_DATUM, LOCAL_BOOKMARK, LOCAL_SCOPE, LOCAL_BLACK
};

union local_union_t {
  struct datum_t datum;
  void* none;
};

struct local_t {
  struct string_t name;
  enum local_tag_t tag;
  union local_union_t data;
};

struct locals_t {
  size_t used, capacity;
  struct local_t* data;
};

enum local_status_t {
  LOCAL_UNDEFINED, LOCAL_DEFINED, LOCAL_BLACKLISTED
};

extern struct locals_t locals;
enum local_status_t has_local(struct string_t name);
void add_local(struct local_t local);
struct datum_t get_local(struct string_t name);
void clear_to_bookmark();
void clear_to_scope();

enum star_status_t {
  STAR_EMPTY, STAR_INHABITED, STAR_DELETED
};

struct star_t {
  enum star_status_t status;
  struct string_t name;
  struct datum_t data;
};

struct stars_t {
  size_t capacity, utilization;
  struct star_t* data;
};

extern struct stars_t stars;
int has_star(struct string_t name);
void add_star(struct star_t star);
struct datum_t get_star(struct string_t name);
