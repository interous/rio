#pragma once
#include "types.h"

struct trace_t {
  struct string_t descriptor, file;
  uint64_t line, col;
};

struct traces_t {
  size_t used, capacity;
  struct trace_t* data;
};

void init_traces();
void push_trace(struct datum_t datum);
void pop_trace();

// Errors relating to the stack
void empty_pop();

// Errors relating specifically to globals
void duplicate_global(struct string_t name);

// Errors relating specifically to locals
void duplicate_local(struct string_t name);
void blacklisted_local(struct string_t name);
void invalid_blacklist(struct string_t name);
void aliased_deletion(struct string_t name);
