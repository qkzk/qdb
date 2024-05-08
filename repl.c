/*
readline library should be available in `/usr/share/readline/`

Compile with :

```sh
gcc -Wall -Wextra -Wpedantic -Wconversion -g repl.c executer.c parser.c lexer.c
-o ./bin/repl -lreadline; ./bin/repl
```
*/
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <readline/history.h>
#include <readline/readline.h>

#include "executer.h"

#define MAXLINE 1024
#define DEBUG false

int main(void) {
  puts("\nWelcome to qdb.\n");
  while (true) {
    char* input = readline("qdb> ");
    add_history(input);
    if (DEBUG) {
      printf("input: %s\n", input);
    }
    if (input == NULL) {
      break;
    }
    execute_request(input);
    free(input);
  }
  puts("bye!\n");
  return 0;
}
