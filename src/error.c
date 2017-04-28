#include "error.h"
#include <stdio.h>

struct traces_t traces;

void init_traces() {
  traces.used = 0;
  traces.capacity = 64;
  traces.data = malloc(traces.capacity * sizeof(struct trace_t));
}

void push_trace(struct datum_t datum) {
  struct trace_t trace;
  trace.file = copy_string(datum.file);
  trace.line = datum.line;
  trace.col = datum.col;
  switch(datum.tag) {
    case VALUE:
      trace.descriptor = copy_string(datum.datum.value.data);
      break;
    case EINT8:
      trace.descriptor = int_to_string(datum.datum.eint8.data);
      break;
    case BLOCK:
      trace.descriptor = string(7, "(block)");
      break;
  }
  traces.data[traces.used++] = trace;
  if(traces.used >= traces.capacity) {
    traces.capacity *= 2;
    traces.data = realloc(traces.data, traces.capacity * sizeof(struct trace_t));
  }
}

void pop_trace() {
  if(traces.used <= 0) {
    printf("invalid pop from trace stack\n");
    exit(-1);
  }
  free(traces.data[traces.used].descriptor.data);
  free(traces.data[traces.used].file.data);
  traces.used--;
  if(traces.used < traces.capacity / 3) {
    traces.capacity /= 2;
    traces.data = realloc(traces.data, traces.capacity * sizeof(struct trace_t));
  }
}

void print_trace() {
  printf("\nStack trace:\n");
  int i;
  for(i = traces.used; i >= 0; i--) {
    printf("  ");
    print_string(traces.data[i].descriptor);
    printf(" ");
    print_string(traces.data[i].file);
    printf(" %lu:%lu\n", traces.data[i].line, traces.data[i].col);
  }
  exit(-1);
}

void empty_pop() {
  printf("pop with empty stack\n");
  print_trace();
}

void duplicate_global(struct string_t name) {
  printf("duplicate global: ");
  print_string(name);
  printf("\n");
  print_trace();
}

void duplicate_local(struct string_t name) {
  printf("duplicate local: ");
  print_string(name);
  printf("\n");
  print_trace();
}

void blacklisted_local(struct string_t name) {
  printf("blacklisted local: ");
  print_string(name);
  printf("\n");
  print_trace();
}

void invalid_blacklist(struct string_t name) {
  printf("invalid blacklist: ");
  print_string(name);
  printf("\n");
  print_trace();
}

void aliased_deletion(struct string_t name) {
  printf("aliased deletion: ");
  print_string(name);
  printf("\n");
  print_trace();
}
