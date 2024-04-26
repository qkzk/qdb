#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXLINE 1000

typedef struct Input_Buffer {
  char* buffer;
  size_t capacity;
  ssize_t bytes_read;
} input_buffer;

input_buffer* create_input_buffer(size_t capacity) {
  input_buffer* input = (input_buffer*)malloc(sizeof(input_buffer));
  assert(input != NULL);

  input->capacity = capacity;
  input->bytes_read = EOF;
  input->buffer = NULL;

  return input;
}

void destroy_input(input_buffer* input) {
  free(input->buffer);
  free(input);
}

void print_prompt(void) {
  printf("db> ");
}

void read_input(input_buffer* input) {
  input->bytes_read = getline(&input->buffer, &input->capacity, stdin);
}

void error_check_input(input_buffer* input) {
  if (input->bytes_read <= 0) {
    fprintf(stderr, "Error reading input\n");
    destroy_input(input);
    exit(EXIT_FAILURE);
  }
  input->bytes_read -= 1;  // remove trailing newlines.
  input->buffer[input->bytes_read] = '\0';
}

void parse_input(input_buffer* input) {
  if (strcmp(input->buffer, ".exit") == 0) {
    printf("Exiting\n");
    destroy_input(input);
    exit(EXIT_SUCCESS);
  } else {
    printf("Unknown command %s\n", input->buffer);
  }
}

int main(void) {
  input_buffer* input = create_input_buffer(MAXLINE);

  while (true) {
    print_prompt();
    read_input(input);
    error_check_input(input);
    parse_input(input);
  }

  destroy_input(input);

  return 0;
}
