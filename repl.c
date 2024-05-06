#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "executer.h"

#define MAXLINE 1024
#define DEBUG false

int main(void) {
  char* input = (char*)malloc(sizeof(char) * MAXLINE);
  assert(input != NULL);
  size_t maxline = MAXLINE;
  ssize_t read_chars = 0;
  printf("\n\nWelcome to qdb.\n\nqdb> ");
  while ((read_chars = getline(&input, &maxline, stdin)) != EOF) {
    size_t input_len = strlen(input);
    input[strcspn(input, "\n")] = '\0';
    if (DEBUG) {
      printf("input: %s\n", input);
    }
    execute_request(input);
    printf("qdb> ");
  }
  printf("done\n");
  return 0;
}
